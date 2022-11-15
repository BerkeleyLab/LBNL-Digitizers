/*
 * Publish monitor values
 */
#include <stdio.h>
#include <string.h>
#include <lwip/udp.h>
#include "autotrim.h"
#include "platform_config.h"
#include "hsdProtocol.h"
#include "cellComm.h"
#include "publisher.h"
#include "afe.h"
#include "evr.h"
#include "gpio.h"
#include "localOscillator.h"
#include "systemParameters.h"
#include "util.h"

#define MAX_CHANNELS_PER_CHAIN (HSD_PROTOCOL_ADC_COUNT/CFG_BPM_COUNT)

#define REG(base,chan)  ((base) + (GPIO_IDX_PER_BPM * (chan)))

static struct udp_pcb *pcb;
static ip_addr_t  subscriberAddr;
static u16_t      subscriberPort;

/*
 * Send values to subscriber
 */
static void
publishSlowAcquisition(unsigned int saSeconds, unsigned int saTicks)
{
    int i;
    int adcChannel, chainNumber;
    struct pbuf *p;
    struct hsdSlowAcquisition *pk;
    static epicsUInt32 packetNumber = 1;
    uint32_t r;
    p = pbuf_alloc(PBUF_TRANSPORT, sizeof *pk, PBUF_RAM);
    if (p == NULL) {
        printf("Can't allocate pbuf for slow data\n");
        return;
    }
    pk = (struct hsdSlowAcquisition *)p->payload;
    pk->packetNumber = packetNumber++;
    pk->seconds = saSeconds;
    pk->ticks = saTicks;
    pk->magic = HSD_PROTOCOL_MAGIC_SLOW_ACQUISITION;
    for (i = 0 ; i < HSD_PROTOCOL_DSP_COUNT ; i++) {
        chainNumber = i;
        pk->xPos[i] = GPIO_READ(REG(GPIO_IDX_POSITION_CALC_SA_X, chainNumber));
        pk->yPos[i] = GPIO_READ(REG(GPIO_IDX_POSITION_CALC_SA_Y, chainNumber));
        pk->skew[i] = GPIO_READ(REG(GPIO_IDX_POSITION_CALC_SA_Q, chainNumber));
        pk->buttonSum[i] = GPIO_READ(REG(GPIO_IDX_POSITION_CALC_SA_S, chainNumber));
        pk->xRMSwide[i] = GPIO_READ(REG(GPIO_IDX_RMS_X_WIDE, chainNumber));
        pk->yRMSwide[i] = GPIO_READ(REG(GPIO_IDX_RMS_Y_WIDE, chainNumber));
        pk->xRMSnarrow[i] = GPIO_READ(REG(GPIO_IDX_RMS_X_NARROW, chainNumber));
        pk->yRMSnarrow[i] = GPIO_READ(REG(GPIO_IDX_RMS_Y_NARROW, chainNumber));
        pk->lossOfBeamStatus[i] = GPIO_READ(REG(GPIO_IDX_LOSS_OF_BEAM_TRIGGER, chainNumber));
        pk->prelimProcStatus[i] = GPIO_READ(REG(GPIO_IDX_PRELIM_STATUS, chainNumber));
    }
    pk->recorderStatus = 0;
    pk->syncStatus = 0;
    pk->clipStatus = rfADCstatus();
    pk->sdSyncStatus = localOscGetSdSyncStatus();
    pk->cellCommStatus = 0;
    pk->autotrimStatus = autotrimStatus(0);
    for (i = 0 ; i < HSD_PROTOCOL_ADC_COUNT ; i++) {
        adcChannel = i % MAX_CHANNELS_PER_CHAIN;
        chainNumber = i / MAX_CHANNELS_PER_CHAIN;
        pk->rfMag[i] = GPIO_READ(REG(GPIO_IDX_PRELIM_RF_MAG_0 +
                adcChannel, chainNumber));
        pk->ptLoMag[i] = GPIO_READ(REG(GPIO_IDX_PRELIM_PT_LO_MAG_0 +
                adcChannel, chainNumber));
        pk->ptHiMag[i] = GPIO_READ(REG(GPIO_IDX_PRELIM_PT_HI_MAG_0 +
                adcChannel, chainNumber));
        pk->gainFactor[i] = GPIO_READ(REG(GPIO_IDX_ADC_GAIN_FACTOR_0 +
                adcChannel, chainNumber));
    }
    r = 0;
    pk->adcPeak[0] = r;
    pk->adcPeak[1] = r >> 16;
    r = 0;
    pk->adcPeak[2] = r;
    pk->adcPeak[3] = r >> 16;
    udp_sendto(pcb, p, &subscriberAddr, subscriberPort);
    pbuf_free(p);
}

/*
 * Publish when appropriate
 */
void
publisherCheck(void)
{
    unsigned int saSeconds;
    unsigned int saTicks;
    static unsigned int previousSaSeconds, previousSaTicks;

    /*
     * Wait for SA time stamp to stabilize
     */
    saTicks = GPIO_READ(GPIO_IDX_SA_TIMESTAMP_TICKS);
    for (;;) {
        unsigned int checkTicks;
        saSeconds = GPIO_READ(GPIO_IDX_SA_TIMESTAMP_SEC);
        checkTicks = GPIO_READ(GPIO_IDX_SA_TIMESTAMP_TICKS);
        if (checkTicks == saTicks) break;
        saTicks = checkTicks;
    }
    if (subscriberPort == 0) {
        previousSaSeconds = saSeconds;
        previousSaTicks = saTicks;
    }
    else {
        if ((saTicks != previousSaTicks) || (saSeconds != previousSaSeconds)) {
            int evrTickDiff = (saTicks - previousSaTicks) +
                                ((saSeconds-previousSaSeconds) * 124910000);
            unsigned int sysMicroseconds = MICROSECONDS_SINCE_BOOT();
            static int sysMicrosecondsOld, evrTickDiffOld, sysTickDiffOld;
            int sysTickDiff = (sysMicroseconds - sysMicrosecondsOld) *
                ((float) 99999001/1000000);
            if ((debugFlags & DEBUGFLAG_SA_TIMING_CHECK)
             && ((evrTickDiff < 11241900) || (evrTickDiff > 13740100)
              || (sysTickDiff < 9700000) || (sysTickDiff > 10200000))) {
                printf("old:%d:%09d  new:%d:%09d "
                       "evrTickDiff:%d sysTickDiff:%d "
                       "evrTickDiffOld:%d sysTickDiffOld:%d\n",
                                             previousSaSeconds, previousSaTicks,
                                             saSeconds, saTicks,
                                             evrTickDiff, sysTickDiff,
                                             evrTickDiffOld, sysTickDiffOld);
            }
            sysMicrosecondsOld = sysMicroseconds;
            evrTickDiffOld = evrTickDiff;
            sysTickDiffOld = sysTickDiff;
            previousSaSeconds = saSeconds;
            previousSaTicks = saTicks;
            publishSlowAcquisition(saSeconds, saTicks);
        }
    }
}

/*
 * Handle a subscription request
 */
static void
publisher_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                   const ip_addr_t *fromAddr, u16_t fromPort)
{
    const char *cp = p->payload;
    static epicsInt16 fofbIndex = -1000;

    /*
     * Must copy payload rather than just using payload area
     * since the latter is not aligned on a 32-bit boundary
     * which results in (silently mangled) misaligned accesses.
     */
    if (debugFlags & DEBUGFLAG_PUBLISHER) {
        long addr = ntohl(fromAddr->addr);
        printf("publisher_callback: %d (%x) from %d.%d.%d.%d:%d\n",
                                                p->len,
                                                p->len >= 1 ? (*cp & 0xFF) : 0,
                                                (int)((addr >> 24) & 0xFF),
                                                (int)((addr >> 16) & 0xFF),
                                                (int)((addr >>  8) & 0xFF),
                                                (int)((addr      ) & 0xFF),
                                                fromPort);
    }
    if (p->len == sizeof fofbIndex) {
        epicsInt16 newIndex;
        memcpy(&newIndex, p->payload, sizeof newIndex);
        if (newIndex != fofbIndex) {
            fofbIndex = newIndex;
            cellCommSetFOFB(fofbIndex);
        }
        subscriberAddr = *fromAddr;
        subscriberPort = fromPort;
    }
    pbuf_free(p);
}

/*
 * Set up publisher UDP port
 */
void
publisherInit(void)
{
    int err;

    pcb = udp_new();
    if (pcb == NULL) {
        fatal("Can't create publisher pcb\n");
        return;
    }
    err = udp_bind(pcb, IP_ADDR_ANY, HSD_PROTOCOL_PUBLISHER_UDP_PORT);
    if (err != ERR_OK) {
        fatal("Can't bind to publisher port, error:%d", err);
        return;
    }
    udp_recv(pcb, publisher_callback, NULL);
}
