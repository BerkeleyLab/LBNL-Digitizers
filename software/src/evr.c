#include <stdio.h>
#include <stdint.h>
#include <xil_io.h>
#include <xparameters.h>
#include "evr.h"
#include "gpio.h"
#include "util.h"

#define EVENT_HEARTBEAT         122
#define EVENT_PPS               125

#define EVR_REG17_BANK_B        0x1
#define EVR_REG17_DB_DISABLE    0x4

#define EVR_REG(r)   (XPAR_EVR_AXI_0_BASEADDR+((r)*sizeof(uint32_t)))
#define EVR_DELAY(t)  EVR_REG((t)*2)
#define EVR_WIDTH(t)  EVR_REG((t)*2+1)

#define EVR_RAM_A(e) (XPAR_EVR_AXI_0_BASEADDR+0x2000+((e)*sizeof(uint32_t)))
#define EVR_RAM_B(e) (XPAR_EVR_AXI_0_BASEADDR+0x4000+((e)*sizeof(uint32_t)))

struct eventCheck {
    int      count;
    uint32_t ticks[2];
};

static int
evChk(const char *name, struct eventCheck *ep)
{
    unsigned int diff;
    int ret = 0;

    switch (ep->count) {
    case 0: printf("No %s events!\n", name);       break;
    case 1: printf("Only one %s event!\n", name);  break;
    default:
        diff = ep->ticks[1] - ep->ticks[0];
        if ((diff <= 900000) || (diff >= 1100000)) {
            printf("Warning -- %s events arriving %u microseconds apart).\n",
                                                                    name, diff);
        }
        else {
            ret = 1;
        }
        break;
    }
    return ret;
}

static void
evGot(struct eventCheck *ep)
{
    if (ep->count < 2) ep->ticks[ep->count] = MICROSECONDS_SINCE_BOOT();
    ep->count++;
}

static struct eventCheck heartbeat, pps;
void
evrInit(void)
{
    int t;
    uint32_t then;
    int firstEvent0 = 1;

    /*
     * Generate and remove reset
     */
    Xil_Out32(EVR_REG(20), 1);
    Xil_Out32(EVR_REG(20), 0);

    /*
     * Disable distributed data buffer
     */
    Xil_Out32(EVR_REG(17), EVR_REG17_DB_DISABLE);

    /*
     * Confirm that heartbeat and PPS markers are present
     */
    heartbeat.count = 0;
    pps.count = 0;
    evrSetEventAction(EVENT_HEARTBEAT, EVR_RAM_WRITE_FIFO);
    evrSetEventAction(EVENT_PPS,       EVR_RAM_WRITE_FIFO);
    then = MICROSECONDS_SINCE_BOOT();
    for (;;) {
        if ((Xil_In32(EVR_REG(28)) & 0x1) == 0) {
            unsigned int seconds = Xil_In32(EVR_REG(29));
            unsigned int ticks = Xil_In32(EVR_REG(30));
            int eventCode = Xil_In32(EVR_REG(31));
            switch(eventCode) {
            case EVENT_HEARTBEAT: evGot(&heartbeat); break;
            case EVENT_PPS:       evGot(&pps);       break;
            default:
                /*
                 * For unknown reasons the event receiver often (always?) 
                 * emits a spurious event 0 on startup.
                 */
                if ((eventCode == 0) && firstEvent0) {
                    firstEvent0 = 0;
                    break;
                }
                printf("Warning -- Unexpected event %d (seconds/ticks:%d/%d)\n",
                                                    eventCode, seconds, ticks);
                break;
            }
        }
        if (((uint32_t)(MICROSECONDS_SINCE_BOOT() - then) > 6000000)
         || ((heartbeat.count >= 2) && (pps.count >= 2))) break;
    }
    evChk("Heartbeat", &heartbeat);
    evChk("PPS", &pps);

    /*
     * Trigger 0 is the heartbeat event marker used
     * to synchronize the SROC reference generation.
     * Make the output nice and wide so we can also use it as a
     * visual 'event receiver active' front panel indicator.
     */
    evrSetTriggerDelay(0, 1);
    evrSetTriggerWidth(0, 125000000 / 4);
    evrSetEventAction(EVENT_HEARTBEAT, EVR_RAM_TRIGGER_0);

    /*
     * Trigger 1 is a 1 pulse per second (exactly) marker.
     * Make the output nice and wide so we can also use it as a
     * visual 'time of day valid' front panel indicator.
     */
    evrSetTriggerDelay(1, 1);
    evrSetTriggerWidth(1, 125000000 / 4);
    evrSetEventAction(EVENT_PPS, EVR_RAM_TRIGGER_1);

    /*
     * Remaining triggers are available as ADC recorder event triggers.
     * Make the outputs nice and wide since for the bunch current monitor
     * the 'injection event' trigger must overlap the 'acquisition event'
     * trigger to indicate that this is a true injection event.
     */
    for (t = 2 ; t < 8 ; t++) {
        evrSetTriggerDelay(t, 1);
        evrSetTriggerWidth(t, 12500000);
    }
}

void
evrShow(void)
{
    int i;
    int activeState = Xil_In32(EVR_REG(16));
    uint32_t csr, action;
    uint16_t actionPresent = 0;
    evrTimestamp ts;

    csr = Xil_In32(EVR_REG(17));
    printf("   Distributed data buffer %sabled.\n",
                                (csr & EVR_REG17_DB_DISABLE) ? "dis" : "en");
    printf("   RAM %s active.\n", (csr & 0x1) ? "B" : "A");
    for (i = 0 ; i < EVR_EVENT_COUNT ; i++) {
        action = evrGetEventAction(i);
        if (action) {
            int b;
            actionPresent |= action;
            printf("   Event %3d: ", i);
            for (b = 15 ; b >= 0 ; b--) {
                int m = 1 << b;
                if (action & m) {
                    switch (b) {
                    case 15:    printf("IRQ");                      break;
                    case 14:    printf("LATCH TIME");               break;
                    case 13:    printf("FIFO");                     break;
                    default: if (b <= 7) { printf("TRG %d", b); }
                             else        { printf("0x%x", 1<<b); }  break;
                    }
                    action &= ~m;
                    if (action == 0) {
                        printf("\n");
                        break;
                    }
                    printf(", ");
                }
            }
        }
    }
    for (i = 0 ; i < EVR_TRIGGER_COUNT ; i++) {
        unsigned int delay = Xil_In32(EVR_DELAY(i));
        unsigned int width = Xil_In32(EVR_WIDTH(i));
        if (actionPresent & (1 << i)) {
            printf("   TRG %d: Delay:%-8d Width:%-8d Active %s\n",
                    i, delay, width, (activeState & (1 << i)) ? "Low" : "High");
        }
    }
    if (evrNoutOfSequenceSeconds())
        printf("  Out of sequence seconds: %d\n", evrNoutOfSequenceSeconds());
    if (evrNtooFewSecondEvents())
        printf("    Too few seconds codes: %d\n", evrNtooFewSecondEvents());
    if (evrNtooManySecondEvents())
        printf("   Too many seconds codes: %d\n", evrNtooManySecondEvents());
    evrCurrentTime(&ts);
    printf("EVR seconds:ticks  %d:%09d\n", ts.secPastEpoch, ts.ticks);
}

void
evrCurrentTime(evrTimestamp *ts)
{
    uint32_t s;

    ts->secPastEpoch = Xil_In32(EVR_REG(24));
    for (;;) {
        ts->ticks = Xil_In32(EVR_REG(25));
        s = Xil_In32(EVR_REG(24));
        if (s == ts->secPastEpoch)
            return;
        ts->secPastEpoch = s;
    }
}

void
evrSetTriggerDelay(unsigned int triggerNumber, int ticks)
{
    if (ticks <= 0) ticks = 1;
    if (triggerNumber < EVR_TRIGGER_COUNT)
        Xil_Out32(EVR_DELAY(triggerNumber), ticks);
}

int
evrGetTriggerDelay(unsigned int triggerNumber)
{
    if (triggerNumber < EVR_TRIGGER_COUNT)
        return Xil_In32(EVR_DELAY(triggerNumber));
    return 0;
}

void
evrSetTriggerWidth(unsigned int triggerNumber, int ticks)
{
    if (triggerNumber < EVR_TRIGGER_COUNT)
        Xil_Out32(EVR_WIDTH(triggerNumber), ticks);
}

void
evrSetEventAction(unsigned int eventNumber, int action)
{
    unsigned int csr = Xil_In32(EVR_REG(17));
    unsigned int addr;

    if (eventNumber < EVR_EVENT_COUNT) {
        addr = (csr & EVR_REG17_BANK_B) ? EVR_RAM_B(eventNumber) :
                                          EVR_RAM_A(eventNumber);
        Xil_Out32(addr, (Xil_In32(addr) & ~0xFFFF) | (action & 0xFFFF));
    }
}

void
evrAddEventAction(unsigned int eventNumber, int action)
{
    evrSetEventAction(eventNumber, action | evrGetEventAction(eventNumber));
}

void
evrRemoveEventAction(unsigned int eventNumber, int action)
{
    evrSetEventAction(eventNumber, ~action & evrGetEventAction(eventNumber));
}

int
evrGetEventAction(unsigned int eventNumber)
{
    unsigned int csr = Xil_In32(EVR_REG(17));
    unsigned int addr;
    int action = 0;

    if (eventNumber < EVR_EVENT_COUNT) {
        addr = (csr & EVR_REG17_BANK_B) ? EVR_RAM_B(eventNumber) :
                                          EVR_RAM_A(eventNumber);
        action = Xil_In32(addr);
    }
    return action & 0xFFFF;
}

unsigned int
evrNoutOfSequenceSeconds(void)
{
    return (Xil_In32(EVR_REG(28)) >> 2) & 0x3FF;
}

unsigned int
evrNtooFewSecondEvents(void)
{
    return (Xil_In32(EVR_REG(28)) >> 12) & 0x3FF;
}

unsigned int
evrNtooManySecondEvents(void)
{
    return (Xil_In32(EVR_REG(28)) >> 22) & 0x3FF;
}
