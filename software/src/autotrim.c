/*
 * Auto-trim support
 */

#include <stdio.h>
#include "autotrim.h"
#include "gpio.h"
#include "util.h"

#define AUTOTRIM_CSR_MODE_MASK              0x7
#define AUTOTRIM_CSR_TIME_MUX_PILOT_PULSES  0x8
#define AUTOTRIM_CSR_STATUS_MASK            0x70
#define AUTOTRIM_CSR_STATUS_SHIFT           4
#define AUTOTRIM_CSR_FILTER_SHIFT_MASK      0x700
#define AUTOTRIM_CSR_FILTER_SHIFT_SHIFT     8
#define AUTOTRIM_CSR_TIME_MUX_SIMULATE_BEAM 0x80000000

#define AUTOTRIM_CSR_STATUS_OFF             0x0
#define AUTOTRIM_CSR_STATUS_NO_TONE         0x1
#define AUTOTRIM_CSR_STATUS_EXCESS_DIFF     0x2
#define AUTOTRIM_CSR_STATUS_ACTIVE          0x3

#define AUTOTRIM_CSR_FILTER_SHIFT_MAX       7

// Must match gateware
#define AUTOTRIM_GAIN_FULL_SCALE            (1<<25)

#define REG(base,chan)  ((base) + (GPIO_IDX_PER_BPM * (chan)))

void
autotrimSetStaticGains(unsigned int bpm, unsigned int channel, int gain)
{
    if (bpm >= CFG_BPM_COUNT) return;
    GPIO_WRITE(REG(GPIO_IDX_ADC_GAIN_FACTOR_0 + channel, bpm),
            gain);
}

void
autotrimUsePulsePilot(unsigned int bpm, int flag)
{
    uint32_t csr;

    if (bpm >= CFG_BPM_COUNT) return;
    csr = GPIO_READ(REG(GPIO_IDX_AUTOTRIM_CSR, bpm));

    if (flag)
        csr |=  AUTOTRIM_CSR_TIME_MUX_PILOT_PULSES;
    else
        csr &= ~AUTOTRIM_CSR_TIME_MUX_PILOT_PULSES;
    GPIO_WRITE(REG(GPIO_IDX_AUTOTRIM_CSR, bpm), csr);
}

static void
setMode(unsigned int bpm, int mode)
{
    uint32_t csr;

    if (bpm >= CFG_BPM_COUNT) return;
    csr = GPIO_READ(REG(GPIO_IDX_AUTOTRIM_CSR, bpm));

    csr &= ~AUTOTRIM_CSR_MODE_MASK;
    csr |= (mode & AUTOTRIM_CSR_MODE_MASK);
    GPIO_WRITE(REG(GPIO_IDX_AUTOTRIM_CSR, bpm), csr);
}

void
autotrimEnable(unsigned int bpm, int mode)
{
    switch (mode) {
    case AUTOTRIM_CSR_MODE_OFF:
    case AUTOTRIM_CSR_MODE_LOW:
    case AUTOTRIM_CSR_MODE_HIGH:
    case AUTOTRIM_CSR_MODE_DUAL:
    case AUTOTRIM_MODE_INBAND:
        setMode(bpm, mode);
        break;
    }
}

int
autotrimStatus(unsigned int bpm)
{
    uint32_t status;

    if (bpm >= CFG_BPM_COUNT) return 0;
    status = GPIO_READ(REG(GPIO_IDX_AUTOTRIM_CSR, bpm));

    return (status & AUTOTRIM_CSR_STATUS_MASK) >> AUTOTRIM_CSR_STATUS_SHIFT;
}

void
autotrimSetThreshold(unsigned int bpm, unsigned int threshold)
{
    if (bpm >= CFG_BPM_COUNT) return;
    GPIO_WRITE(REG(GPIO_IDX_AUTOTRIM_THRESHOLD, bpm), threshold);
}

unsigned int
autotrimGetThreshold(unsigned int bpm)
{
    if (bpm >= CFG_BPM_COUNT) return 0;
    return GPIO_READ(REG(GPIO_IDX_AUTOTRIM_THRESHOLD, bpm));
}

void
autotrimSetFilterShift(unsigned int bpm, unsigned int filterShift)
{
    uint32_t csr;

    if (bpm >= CFG_BPM_COUNT) return;
    csr = GPIO_READ(REG(GPIO_IDX_AUTOTRIM_CSR, bpm));

    csr &= ~AUTOTRIM_CSR_FILTER_SHIFT_MASK;
    csr |= (filterShift << AUTOTRIM_CSR_FILTER_SHIFT_SHIFT) &
        AUTOTRIM_CSR_FILTER_SHIFT_MASK;
    GPIO_WRITE(REG(GPIO_IDX_AUTOTRIM_CSR, bpm), csr);
}

unsigned int
autotrimGetFilterShift(unsigned int bpm)
{
    uint32_t csr;

    if (bpm >= CFG_BPM_COUNT) return 0;
    csr = GPIO_READ(REG(GPIO_IDX_AUTOTRIM_CSR, bpm));
    return (csr & AUTOTRIM_CSR_FILTER_SHIFT_MASK) >>
        AUTOTRIM_CSR_FILTER_SHIFT_SHIFT;
}
