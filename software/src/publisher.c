/*
 * Publish monitor values
 */
#include <stdio.h>
#include <string.h>
#include <lwip/udp.h>
#include "autotrim.h"
#include "platform_config.h"
#include "hsdProtocol.h"
#include "publisher.h"
#include "afe.h"
#include "evr.h"
#include "gpio.h"
#include "localOscillator.h"
#include "systemParameters.h"
#include "util.h"

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
    pk->xPos = 0;
    pk->yPos = 0;
    pk->skew = 0;
    pk->buttonSum = 0;
    pk->xRMSwide = 0;
    pk->yRMSwide = 0;
    pk->xRMSnarrow = 0;
    pk->yRMSnarrow = 0;
    pk->recorderStatus = 0;
    pk->syncStatus = 0;
    pk->clipStatus = 0;
    pk->sdSyncStatus = 0;
    pk->cellCommStatus = 0;
    pk->autotrimStatus = 0;
    for (i = 0 ; i < HSD_PROTOCOL_ADC_COUNT ; i++) {
        pk->rfMag[i] = GPIO_READ(GPIO_IDX_PRELIM_RF_MAG_0+i);
        pk->ptLoMag[i] = GPIO_READ(GPIO_IDX_PRELIM_PT_LO_MAG_0+i);
        pk->ptHiMag[i] = GPIO_READ(GPIO_IDX_PRELIM_PT_HI_MAG_0+i);
        pk->gainFactor[i] = GPIO_READ(GPIO_IDX_ADC_GAIN_FACTOR_0+i);
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
            unsigned int sysTicks = MICROSECONDS_SINCE_BOOT();
            static int sysTicksOld, evrTickDiffOld, sysTickDiffOld;
            int sysTickDiff = sysTicks - sysTicksOld;
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
            sysTicksOld = sysTicks;
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