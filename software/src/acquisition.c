/*
 * Data acquisition
 */
#include <stdio.h>
#include <stdint.h>
#include "acquisition.h"
#include "afe.h"
#include "gpio.h"
#include "util.h"

#ifdef VERILOG_FIRMWARE_STYLE_HSD

#define CSR_W_ARM                       0x80000000
#define CSR_R_ACQ_ACTIVE                0x80000000
#define CSR_R_FULL                      0x40000000

#define TRIGGER_CONFIG_LEVEL_MASK       0x00FFFF
#define TRIGGER_CONFIG_ENABLES_MASK     0x7F0000
#define TRIGGER_CONFIG_ENABLES_SHIFT    16
#define TRIGGER_CONFIG_FALLING_EDGE     0x800000
#define TRIGGER_CONFIG_SEGMODE_MASK     0x60000000
#define TRIGGER_CONFIG_SEGMODE_SHIFT    29
#define TRIGGER_CONFIG_BONDED           0x80000000

#define REG(base,chan)  ((base) + (GPIO_IDX_PER_ADC * (chan)))

/*
 * Trigger response latency and multiple samples per clock cause the '-14'.
 */
#define CONTINUOUS_ACQUISITION_SAMPLES (CFG_ACQUISITION_BUFFER_CAPACITY - 14)
#define LONG_SEGMENT_SAMPLES (CFG_LONG_SEGMENT_CAPACITY - 14)
#define SHORT_SEGMENT_SAMPLES (CFG_SHORT_SEGMENT_CAPACITY - 14)
#define LONG_SEGMENT_COUNT (CFG_ACQUISITION_BUFFER_CAPACITY / \
                                                      CFG_LONG_SEGMENT_CAPACITY)
#define SHORT_SEGMENT_COUNT (CFG_ACQUISITION_BUFFER_CAPACITY / \
                                                     CFG_SHORT_SEGMENT_CAPACITY)
#define ADC_CLK_PER_LONG_SEGMENT (CFG_LONG_SEGMENT_CAPACITY / \
                                                      CFG_AXI_SAMPLES_PER_CLOCK)
#define ADC_CLK_PER_SHORT_SEGMENT (CFG_SHORT_SEGMENT_CAPACITY / \
                                                      CFG_AXI_SAMPLES_PER_CLOCK)

static struct acqConfig {
    uint32_t    triggerReg;
    int         pretriggerCount;
    int         pretriggerClocks;
    enum {SEGMODE_CONTIGUOUS=0, SEGMODE_LONG_SEGMENTS, SEGMODE_SHORT_SEGMENTS}
                segMode;
    uint32_t    earlySegmentInterval;
    uint32_t    laterSegmentInterval;
} acqConfig[CFG_ADC_CHANNEL_COUNT];

void
acquisitionInit(void)
{
    int channel;
    for (channel = 0 ; channel < CFG_ADC_CHANNEL_COUNT ; channel++) {
        acquisitionSetPretriggerCount(channel, 1);
        acquisitionSetTriggerLevel(channel, 10000000);
    }
}

/*
 * Hook for acquisition polling.
 * Empty for generic HSD and BRAM acquisition.
 */
void
acquisitionCrank(void) { }

void
acquisitionArm(int channel, int enable)
{
    uint32_t csr, config1, config2;
    struct acqConfig *ap;

    if ((channel < 0) || (channel >= CFG_ADC_CHANNEL_COUNT)) return;
    ap = &acqConfig[channel];
    if (enable) {
        csr = CSR_W_ARM;
        if (ap->segMode == SEGMODE_CONTIGUOUS) {
            config1 = ap->pretriggerClocks;
            config2 = 0;
        }
        else {
            int clk_PerSeg = (ap->segMode == SEGMODE_LONG_SEGMENTS) ?
                           ADC_CLK_PER_LONG_SEGMENT : ADC_CLK_PER_SHORT_SEGMENT;
            int earlySegmentGap = ap->earlySegmentInterval - clk_PerSeg;
            int laterSegmentGap = ap->laterSegmentInterval - clk_PerSeg;
            config1 = ((earlySegmentGap - 2) << 18) | ap->pretriggerClocks;
            config2 = laterSegmentGap - 2;
        }
        GPIO_WRITE(REG(GPIO_IDX_ADC_0_CONFIG_1, channel), config1);
        GPIO_WRITE(REG(GPIO_IDX_ADC_0_CONFIG_2, channel), config2);
    }
    else {
        csr = 0;
    }
    GPIO_WRITE(REG(GPIO_IDX_ADC_0_CSR, channel), csr);
    if (debugFlags & DEBUGFLAG_ACQUISITION) {
        printf("%srm %d\n", enable ? "A" : "Disa", channel);
    }
}


uint32_t
acquisitionStatus(void)
{
    int channel;
    uint32_t csr, base_csr = 0;
    uint32_t v = 0;

    for (channel = 0 ; channel < CFG_ADC_CHANNEL_COUNT ; channel++) {
        if (((channel % CFG_ADCS_PER_BONDED_GROUP) == 0)
         || ((acqConfig[channel].triggerReg & TRIGGER_CONFIG_BONDED) == 0)) {
            csr = GPIO_READ(REG(GPIO_IDX_ADC_0_CSR, channel));
            if ((channel % CFG_ADCS_PER_BONDED_GROUP) == 0) {
                base_csr = csr;
            }
        }
        else {
            csr = base_csr;
        }
        if (csr & CSR_R_ACQ_ACTIVE) v |= 1 << channel;
        if (csr & CSR_R_FULL) v |= 0x10000 << channel;
    }
    if ((debugFlags & DEBUGFLAG_ACQUISITION) && (v != 0)) {
        printf("FULL %02X  ACTIVE %02x\n", ((int)v >> 16),(int)v & 0xFFFF);
    }
    return v;
}

static int
fetch(int csr_idx, int data_idx, int dataLocation)
{
    GPIO_WRITE(csr_idx, dataLocation);
    return GPIO_READ(data_idx);
}

/*
 * Find location of data in RAM
 */
static int
dataLocation(int segMode, int base, int offset)
{
    int off, limit;
    if (segMode == SEGMODE_CONTIGUOUS) {
        off = offset;
        limit = CONTINUOUS_ACQUISITION_SAMPLES;
    }
    else {
        int samplesPerSegment = (segMode == SEGMODE_LONG_SEGMENTS) ?
                                   LONG_SEGMENT_SAMPLES : SHORT_SEGMENT_SAMPLES;
        int segmentCapacity = (segMode == SEGMODE_LONG_SEGMENTS) ?
                         CFG_LONG_SEGMENT_CAPACITY : CFG_SHORT_SEGMENT_CAPACITY;
        int segmentCount = (segMode == SEGMODE_LONG_SEGMENTS) ?
                                       LONG_SEGMENT_COUNT : SHORT_SEGMENT_COUNT;
        int segment = offset / samplesPerSegment;
        int idx = offset % samplesPerSegment;
        off = (segment * segmentCapacity) + idx;
        limit = samplesPerSegment * segmentCount;
    }
    if (offset >= limit) {
        return -1;
    }
    return (base + off) % CFG_ACQUISITION_BUFFER_CAPACITY;
}

int
acquisitionFetch(uint32_t *buf, int capacity, int channel, int offset, int last)
{
    int csr_idx;
    int data_idx;
    int triggerChannel;
    int triggerLocation, base;
    int segMode;
    uint32_t csr;
    int n = 0;

    if ((channel < 0) || (channel >= CFG_ADC_CHANNEL_COUNT) || (capacity < 5)) {
        return 0;
    }
    if (acqConfig[channel].triggerReg & TRIGGER_CONFIG_BONDED) {
        triggerChannel = channel - (channel % CFG_ADCS_PER_BONDED_GROUP);
    }
    else {
        triggerChannel = channel;
    }
    csr = GPIO_READ(REG(GPIO_IDX_ADC_0_CSR, triggerChannel));
    if (csr & CSR_R_ACQ_ACTIVE) {
        return 0;
    }
    csr_idx = REG(GPIO_IDX_ADC_0_CSR, channel);
    data_idx = REG(GPIO_IDX_ADC_0_DATA, channel);
    segMode = acqConfig[triggerChannel].segMode;
    triggerLocation = GPIO_READ(REG(GPIO_IDX_ADC_0_TRIGGER_LOCATION, triggerChannel));
    base = (triggerLocation - acqConfig[triggerChannel].pretriggerCount +
             CFG_ACQUISITION_BUFFER_CAPACITY) % CFG_ACQUISITION_BUFFER_CAPACITY;
    while ((n < capacity) && (offset < last)) {
        int loc;
        if (offset == 0) {
            if (debugFlags & DEBUGFLAG_ACQUISITION) {
                printf("Chan:%d(t%d) trigger@%d (%d:%d)\n", channel,
                                   triggerChannel,
                                   triggerLocation,
                                   triggerLocation / CFG_AXI_SAMPLES_PER_CLOCK,
                                   triggerLocation % CFG_AXI_SAMPLES_PER_CLOCK);
            }
            *buf++ = GPIO_READ(REG(GPIO_IDX_ADC_0_SECONDS, triggerChannel));
            *buf++ = GPIO_READ(REG(GPIO_IDX_ADC_0_TICKS, triggerChannel));
            n = afeFetchCalibration(channel, buf);
            if (n == 0) return 0;
            buf += n;
            n += 2;
        }
        loc = dataLocation(segMode, base, offset);
        if (loc < 0) {
            break;
        }
        *buf++ = fetch(csr_idx, data_idx, loc);
        n++;
        offset++;
    }
    return n;
}

static void
setTrigger(int channel, uint32_t mask, uint32_t v)
{
    uint32_t triggerReg;

    if ((channel < 0) || (channel >= CFG_ADC_CHANNEL_COUNT)) return;
    triggerReg = acqConfig[channel].triggerReg;
    triggerReg &= ~mask;
    triggerReg |= v & mask;
    GPIO_WRITE(REG(GPIO_IDX_ADC_0_TRIGGER_CONFIG, channel), triggerReg);
    if (debugFlags & DEBUGFLAG_ACQUISITION) {
        printf("Trigger %d config: %08X\n", channel, triggerReg);
    }
    acqConfig[channel].triggerReg = triggerReg;
}

/* Trigger levels require special handling */
static int triggerLevels[CFG_ADC_CHANNEL_COUNT];
void
acquisitionSetTriggerLevel(int channel, int microvolts)
{
    int counts;

    triggerLevels[channel] = microvolts;
    counts = afeMicrovoltsToCounts(channel, microvolts);
    if (debugFlags & DEBUGFLAG_CALIBRATION) {
        printf("C%d uV:%d adc:%d\n", channel, microvolts, counts);
    }
    setTrigger(channel, TRIGGER_CONFIG_LEVEL_MASK, counts);

}
void
acquisitionScaleChanged(int channel)
{
    acquisitionSetTriggerLevel(channel, triggerLevels[channel]);
}

void
acquisitionSetTriggerEdge(int channel, int v)
{
    setTrigger(channel, TRIGGER_CONFIG_FALLING_EDGE, v ?
                                               TRIGGER_CONFIG_FALLING_EDGE : 0);
}

void
acquisitionSetTriggerEnables(int channel, int enables)
{
    setTrigger(channel, TRIGGER_CONFIG_ENABLES_MASK,
                                       enables << TRIGGER_CONFIG_ENABLES_SHIFT);
}

void
acquisitionSetBonding(int channel, int bonded)
{
    setTrigger(channel, TRIGGER_CONFIG_BONDED, bonded?TRIGGER_CONFIG_BONDED:0);
}

void
acquisitionSetPretriggerCount(int channel, int n)
{
    int limit = CFG_ACQUISITION_BUFFER_CAPACITY - CFG_AXI_SAMPLES_PER_CLOCK;
    if ((channel < 0) || (channel >= CFG_ADC_CHANNEL_COUNT)) return;
    if (n <= 0) n = 1;
    if (n > limit) {
        n = limit;
    }
    acqConfig[channel].pretriggerCount = n;
    /*
     * The IOC set set up to request at least 1 pretrigger sample.
     */
    acqConfig[channel].pretriggerClocks = (n + CFG_AXI_SAMPLES_PER_CLOCK - 1) /
                                                      CFG_AXI_SAMPLES_PER_CLOCK;
}

void
acquisitionSetSegmentedMode(int channel, int segMode)
{
    if ((channel < 0) || (channel >= CFG_ADC_CHANNEL_COUNT)) return;
    switch (segMode) {
    case SEGMODE_CONTIGUOUS:     break;
    case SEGMODE_LONG_SEGMENTS:  break;
    case SEGMODE_SHORT_SEGMENTS: break;
    default: return;
    }
    acqConfig[channel].segMode = segMode;
    setTrigger(channel, TRIGGER_CONFIG_SEGMODE_MASK,
                                       segMode << TRIGGER_CONFIG_SEGMODE_SHIFT);
}

void
acquisitionSetEarlySegmentInterval(int channel, int adcClockTicks)
{
    if ((channel < 0) || (channel >= CFG_ADC_CHANNEL_COUNT)) return;
    acqConfig[channel].earlySegmentInterval = adcClockTicks;
}

void
acquisitionSetLaterSegmentInterval(int channel, int adcClockTicks)
{
    if ((channel < 0) || (channel >= CFG_ADC_CHANNEL_COUNT)) return;
    acqConfig[channel].laterSegmentInterval = adcClockTicks;
}

void acquisitionSetSize(unsigned int n) { }
void acquisitionSetPassCount(unsigned int n) { }
#endif

#ifdef VERILOG_FIRMWARE_STYLE_BRAM
#include "evr.h"
#define CSR_W_RUN               0x80000000
#define CSR_R_RUNNING           0x80000000
#define CSR_R_ACQ_ADDR_MASK     0x7FFF0000
#define CSR_R_ACQ_ADDR_SHIFT    16

static int haveNewData;
static struct evrTimestamp whenStarted;

void
acquisitionInit(void)
{
    int channel;
    for (channel = 0 ; channel < CFG_ADC_CHANNEL_COUNT ; channel++) {
        acquisitionSetPretriggerCount(channel, 1);
        acquisitionSetTriggerLevel(channel, 10000000);
    }
}

/*
 * Hook for acquisition polling.
 * Empty for generic HSD and BRAM acquisition.
 */
void
acquisitionCrank(void) { }

void
acquisitionArm(int channel, int enable)
{
    if (channel == 0) {
            GPIO_WRITE(GPIO_IDX_ADC_0_CSR, CSR_W_RUN);
        evrCurrentTime(&whenStarted);
        microsecondSpin(20);
        GPIO_WRITE(GPIO_IDX_ADC_0_CSR, 0);
        haveNewData = 1;
    }
}

uint32_t
acquisitionStatus(void)
{
    int channel;
    uint32_t v = 0;
    if (haveNewData) {
        for (channel = 0 ; channel < CFG_ADC_CHANNEL_COUNT ; channel++) {
            v |= 0x10000 << channel;
        }
    }
    return v;
}
int
acquisitionFetch(uint32_t *buf, int capacity, int channel, int offset, int last)
{
    int base = (GPIO_READ(GPIO_IDX_ADC_0_CSR) & CSR_R_ACQ_ADDR_MASK) >> CSR_R_ACQ_ADDR_SHIFT;
    int n = 0;

    if ((GPIO_READ(GPIO_IDX_ADC_0_CSR) &  CSR_R_RUNNING)
     || (channel < 0)
     || (channel >= CFG_ADC_CHANNEL_COUNT)
     || (capacity < 5)) {
        return 0;
    }
    haveNewData = 0;
    while ((n < capacity) && (offset < last)) {
        int addr, muxSel;
        uint32_t s;
        if (offset == 0) {
            *buf++ = whenStarted.secPastEpoch;
            *buf++ = whenStarted.ticks;
            n = afeFetchCalibration(channel, buf);
            if (n == 0) return 0;
            buf += n;
            n += 2;
        }
        addr = (base + (offset >> 3)) % (CFG_ACQUISITION_BUFFER_CAPACITY >> 3);
        muxSel = offset & 0x7;
        GPIO_WRITE(GPIO_IDX_ADC_0_CSR, (((addr * CFG_ADC_CHANNEL_COUNT) + channel) *
                                                        CFG_AXI_SAMPLES_PER_CLOCK) + muxSel);
        s = GPIO_READ(GPIO_IDX_ADC_0_CSR) & 0xFFF0;
        if (offset & 0x1) {
            *buf++ |= s << 16;
            n++;
        }
        else {
            *buf = s;
        }
        offset++;
    }
    return n;
}

void acquisitionScaleChanged(int channel) { }
void acquisitionSetTriggerEdge(int channel, int v){ }
void acquisitionSetTriggerLevel(int channel, int microvolts){ }
void acquisitionSetTriggerEnables(int channel, int mask){ }
void acquisitionSetBonding(int channel, int bonded){ }
void acquisitionSetPretriggerCount(int channel, int n){ }
void acquisitionSetSegmentedMode(int channel, int isSegmented){ }
void acquisitionSetEarlySegmentInterval(int channel, int adcClockTicks){ }
void acquisitionSetLaterSegmentInterval(int channel, int adcClockTicks){ }
void acquisitionSetSize(unsigned int n) { }
void acquisitionSetPassCount(unsigned int n) { }
#endif

#ifdef VERILOG_FIRMWARE_STYLE_BCM

#include <stdio.h>
#include <stdint.h>
#include "acquisition.h"
#include "afe.h"
#include "bcmProtocol.h"
#include "gpio.h"
#include "rfadc.h"
#include "util.h"

#define EVENT_TRIGGER_TIMEOUT_US 3000000

#define DEBUGFLAG_SOFT_TRIGGER 0x100000
#define DEBUGFLAG_READ_TIMEOUT 0x200000

#define ACQ_CHANNEL_COUNT       2

#define CSR_W_START             0x80000000
#define CSR_RW_SOFT_TRIGGER     0x40000000

#define CSR_R_ACTIVE            0x80000000
#define CSR_R_FOLLOWS_INJECTION 0x20000000

#define CSR_SAMPLE_COUNT_RELOAD_MASK    0x3FFF
#define CSR_PASS_COUNT_RELOAD_SHIFT     14
#define CSR_PASS_COUNT_RELOAD_MASK      (((CFG_MAX_PASSES_PER_ACQUISITION << \
                                         1) - 1) << CSR_PASS_COUNT_RELOAD_SHIFT)

#define ADDR_CHANNEL_SHIFT              24
#define ADDR_DPRAM_ADDRESS_SHIFT        3

#if ((1UL << CSR_PASS_COUNT_RELOAD_SHIFT) != \
    (CFG_ACQUISITION_BUFFER_CAPACITY >> ADDR_DPRAM_ADDRESS_SHIFT))
# error("ACQUISITION CSR DOESN'T MATCH config.h")
#endif
#if (CFG_MAX_PASSES_PER_ACQUISITION & (CFG_MAX_PASSES_PER_ACQUISITION - 1))
# error("CFG_MAX_PASSES_PER_ACQUISITION IS NOT A POWER OF 2")
#endif

#define CSR_WRITE(x) GPIO_WRITE(GPIO_IDX_ACQUISITION_CSR, (x))
#define CSR_READ() GPIO_READ(GPIO_IDX_ACQUISITION_CSR)
#define ADDR_WRITE(x) GPIO_WRITE(GPIO_IDX_ACQUISITION_READOUT, (x))
#define DATA_READ() GPIO_READ(GPIO_IDX_ACQUISITION_READOUT)

#define MUX_CHANNEL_MASK    0x3
#define MUX_CHANNEL_SHIFT   4
#define ACQ_DATA_CHANNEL    0
#define ACQ_TRIG_CHANNEL    1

#define SYSREF_CSR_ADC_CLK_FAULT            (1 << 31)
#define SYSREF_CSR_ADC_CLK_DIVISOR_SHIFT    16
#define SYSREF_CSR_REF_CLK_FAULT            (1 << 15)
#define SYSREF_CSR_REF_CLK_DIVISOR_SHIFT    0

static uint32_t size = CFG_ACQUISITION_BUFFER_CAPACITY, acqSize;
static uint32_t passCount = CFG_MAX_PASSES_PER_ACQUISITION, acqPassCount;
static uint32_t usWhenStarted;
static int wasStarted = 0;

#define NEED_SIZE       0x1
#define NEED_PASSCOUNT  0x2
static int needParameters = NEED_SIZE | NEED_PASSCOUNT;

void
acquisitionInit(void)
{
}

/*
 * Scale acquired values to signed 16 bit range
 */
static int
scaleValue(uint32_t v)
{
    return (int32_t)v / (int32_t)acqPassCount;
}

static void
acquisitionStart(void)
{
    uint32_t passCountReload, sampleCountReload;
    static int oldAcqSize;

    acqPassCount = passCount;
    passCountReload = acqPassCount - 2;
    acqSize = size;
    sampleCountReload = (acqSize / CFG_AXI_SAMPLES_PER_CLOCK) - 2;
    if (!(CSR_READ() & CSR_R_ACTIVE)) {
        uint32_t csr = ((passCountReload << CSR_PASS_COUNT_RELOAD_SHIFT) &
                                                   CSR_PASS_COUNT_RELOAD_MASK) |
                       (sampleCountReload & CSR_SAMPLE_COUNT_RELOAD_MASK);
        if (acqSize != oldAcqSize) {
            /*
             * Allow time for acquisition size change to take effect
             */
          CSR_WRITE(csr);
          oldAcqSize = acqSize;
          microsecondSpin(50);
        }
        CSR_WRITE(CSR_W_START | csr);
        usWhenStarted = MICROSECONDS_SINCE_BOOT();
        wasStarted = 1;
    }
}

/*
 * Soft trigger if inactive too long.
 * Clear acquisition if idle more than 5 seconds.
 */
void
acquisitionCrank(void)
{
    uint32_t csr;
    uint32_t now;
    static int firstTime = 1;
    static uint32_t usWhenFinished;

    if (needParameters) return;
    if (firstTime) {
        firstTime = 0;
        acquisitionStart();
    }
    now = MICROSECONDS_SINCE_BOOT();
    csr = CSR_READ();
    if (csr & CSR_R_ACTIVE) {
        if (!(csr & CSR_RW_SOFT_TRIGGER)
         && ((now - usWhenStarted) > EVENT_TRIGGER_TIMEOUT_US)) {
            CSR_WRITE(CSR_RW_SOFT_TRIGGER);
            if (debugFlags & DEBUGFLAG_SOFT_TRIGGER) {
                printf("==== Soft trigger\n");
            }
        }
    }
    else {
        if (wasStarted) {
            wasStarted = 0;
            usWhenFinished = now;
            if (debugFlags & DEBUGFLAG_READ_TIMEOUT) {
                printf("Acq: %d us\n", usWhenFinished - usWhenStarted);
            }
        }
        if ((now - usWhenFinished) > 5000000) {
            if (debugFlags & DEBUGFLAG_READ_TIMEOUT) {
                printf("==== Cancel readout\n");
            }
            acquisitionStart();
        }
    }
}

/*
 * Read values from acquisition buffer
 */
int
acquisitionFetch(uint32_t *buf, int capacity, int channel, int offset, int last)
{
    uint32_t csr = CSR_READ();
    unsigned int count = 0;

    if (capacity < 20) return 0;
    if (!(csr & CSR_R_ACTIVE)) {
        if (offset == 0) {
            uint32_t acqStatus = rfADCstatus();
            uint32_t *base = buf;
            *buf++ = GPIO_READ(GPIO_IDX_ACQUISITION_SECONDS);
            *buf++ = GPIO_READ(GPIO_IDX_ACQUISITION_TICKS);
            /* Steal some unused bits in the RF ADC status word */
            if (csr & CSR_R_FOLLOWS_INJECTION) {
                acqStatus |= BCM_PROTOCOL_ACQ_FOLLOWS_INJECTION;
            }
            if (csr & CSR_RW_SOFT_TRIGGER) {
                acqStatus |= BCM_PROTOCOL_ACQ_SOFTWARE_TRIGGER;
            }
            *buf++ = acqStatus;
            for (channel = 0 ; channel < ACQ_CHANNEL_COUNT ; channel++) {
                buf += afeFetchCalibration(channel, buf);
            }
            count = buf - base;

        }
        while ((count < capacity) && (offset < acqSize)) {
            int sample = offset % CFG_AXI_SAMPLES_PER_CLOCK;
            int dpram = offset / CFG_AXI_SAMPLES_PER_CLOCK;
            uint32_t v = 0;
            int channel;
            for (channel = 0 ; channel < ACQ_CHANNEL_COUNT ; channel++) {
                int16_t s;
                uint32_t readAddr = (channel << ADDR_CHANNEL_SHIFT) |
                                    (dpram << ADDR_DPRAM_ADDRESS_SHIFT) |
                                     sample;
                ADDR_WRITE(readAddr);
                s = scaleValue(DATA_READ());
                v |= (s & 0xFFFF) << (channel * 16);
            }
            *buf++ = v;
            offset++;
            count++;
        }
        if (offset == acqSize) {
            acquisitionStart();
        }
    }
    return count;
}

/*
 * Set acquisition controls
 */
void
acquisitionSetSize(unsigned int n)
{
    if (n < 100) {
        n = 100;
    }
    else if (n > CFG_ACQUISITION_BUFFER_CAPACITY) {
        n = CFG_ACQUISITION_BUFFER_CAPACITY;
    }
    size = n;
    needParameters &= ~NEED_SIZE;

}

void
acquisitionSetPassCount(unsigned int n)
{
    if (n == 0) {
        n = 1;
    }
    else if (n > CFG_MAX_PASSES_PER_ACQUISITION) {
        n = CFG_MAX_PASSES_PER_ACQUISITION;
    }
    passCount = n;
    needParameters &= ~NEED_PASSCOUNT;
}

void acquisitionScaleChanged(int channel) { }
void acquisitionSetTriggerEdge(int channel, int v){ }
void acquisitionSetTriggerLevel(int channel, int microvolts){ }
void acquisitionSetTriggerEnables(int channel, int mask){ }
void acquisitionSetBonding(int channel, int bonded){ }
void acquisitionSetPretriggerCount(int channel, int n){ }
void acquisitionSetSegmentedMode(int channel, int isSegmented){ }
void acquisitionSetEarlySegmentInterval(int channel, int adcClockTicks){ }
void acquisitionSetLaterSegmentInterval(int channel, int adcClockTicks){ }

#endif
