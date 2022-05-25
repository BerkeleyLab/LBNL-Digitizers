#include <stdio.h>
#include "frequencyMonitor.h"
#include "gpio.h"
#include "util.h"

#define EVR_SYNC_CSR_PPS_VALID 0x4

int
frequencyMonitorUsingPPS(void)
{
    uint32_t csr = GPIO_READ(GPIO_IDX_EVR_SYNC_CSR);
    return (csr & EVR_SYNC_CSR_PPS_VALID) != 0;
}

/*
 * Measure clock frequency
 * Go through some contortions to avoid floating point arithmetic when
 * PPS is missing and hence reference interval is SYSCLK / (1 << 27).
 */
unsigned int
frequencyMonitorGet(unsigned int channel)
{
    unsigned int rate;
    GPIO_WRITE(GPIO_IDX_FREQ_MONITOR_CSR, channel);
    rate = GPIO_READ(GPIO_IDX_FREQ_MONITOR_CSR);
    if (!frequencyMonitorUsingPPS()) {
        int ratio = (rate +  ((1 << 27) / 2000)) / ((1 << 27) / 1000);
        rate = 100000 * ratio;
    }
    return rate;
}
