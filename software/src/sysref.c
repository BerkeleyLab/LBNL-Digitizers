#include <stdio.h>
#include <stdint.h>
#include "sysref.h"
#include "gpio.h"
#include "util.h"

#define SYSREF_CSR_ADC_CLK_FAULT            (1 << 31)
#define SYSREF_CSR_ADC_CLK_DIVISOR_SHIFT    16
#define SYSREF_CSR_REF_CLK_FAULT            (1 << 15)
#define SYSREF_CSR_REF_CLK_DIVISOR_SHIFT    0

void
sysrefInit(void)
{
    GPIO_WRITE(GPIO_IDX_SYSREF_CSR,
               ((ADC_CLK_PER_SYSREF-1) << SYSREF_CSR_ADC_CLK_DIVISOR_SHIFT) |
               ((REFCLK_OUT_PER_SYSREF-1) << SYSREF_CSR_REF_CLK_DIVISOR_SHIFT));
    GPIO_WRITE(GPIO_IDX_SYSREF_CSR, SYSREF_CSR_ADC_CLK_FAULT |
                                    SYSREF_CSR_REF_CLK_FAULT);
}

void
sysrefShow(void)
{
    uint32_t v = GPIO_READ(GPIO_IDX_SYSREF_CSR);
    printf("%d ADC AXI clocks per SYSREF (expect %d).\n",
                           ((v >> SYSREF_CSR_ADC_CLK_DIVISOR_SHIFT) & 0x3FF) + 1,
                           ADC_CLK_PER_SYSREF);
    printf("%d FPGA_REFCLK_OUT_C clocks per SYSREF (expect %d).\n",
                           ((v >> SYSREF_CSR_REF_CLK_DIVISOR_SHIFT) & 0x3FF) + 1,
                           REFCLK_OUT_PER_SYSREF);
    if (v & SYSREF_CSR_ADC_CLK_FAULT) {
        print("ADC AXI SYSREF fault.\n");
        GPIO_WRITE(GPIO_IDX_SYSREF_CSR, SYSREF_CSR_ADC_CLK_FAULT);
    }
    if (v & SYSREF_CSR_REF_CLK_FAULT) {
        print("FPGA_REFCLK_OUT_C SYSREF fault.\n");
        GPIO_WRITE(GPIO_IDX_SYSREF_CSR, SYSREF_CSR_REF_CLK_FAULT);
    }
}
