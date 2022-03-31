/*
 * MMCM reconfiguration
 * See PG065, "Clocking Wizard v6.0" for details.
 *
 * Allows build time settings of ADC MMCM to be overridden.  This is handy
 * for applications like the bunch current monitor since it provides a
 * different ADC rate without rebuilding the firmware.
 */

#include <stdio.h>
#include <xil_io.h>
#include "gpio.h"
#include "mmcm.h"
#include "util.h"

#define RD(b,o) Xil_In32((b)+(o))
#define WR(b,o,v) Xil_Out32((b)+(o), (v))

static void
showMMCMreg(uint32_t base, int offset, const char *name)
{
    uint32_t v = RD(base, offset);
    printf("%20s %3.3X: %4.4x:%4.4X\n", name, offset, v>>16, v&0xFFFF);
}

static void
mmcmDump(uint32_t base)
{
    int i;
    uint32_t v;

    showMMCMreg(base, 0x4, "Status");
    v = RD(base, 0x04);
    printf("%26s%socked\n", "", (v & 0x1) ? "L" : "Unl");
    showMMCMreg(base, 0x8, "Error status");
    showMMCMreg(base, 0xC, "Interrupt status");
    showMMCMreg(base, 0x204, "CLKFBOUT_PHASE");
    showMMCMreg(base, 0x25C, "Configuration 23");
    v = RD(base, 0x200);
    printf("Input divide: %d\n", v & 0xFF);
    printf("     Multiply: %d.%03d\n", (v >> 8) & 0xFF, (v >> 16) & 0x3FF);
    for (i = 0 ; i < 8 ; i++) {
        const char *s;
        printf("  CLKOUT%d\n", i);
        v = RD(base,  0x208 + (i * 12));
        printf("     Divide: %d.%03d\n", v & 0xFF, (v >> 8) & 0x3FF);
        v = RD(base,  0x20C + (i * 12));
        if ((int32_t)v < 0) {
            s = "-";
            v = -v;
        }
        else {
            s = "";
        }
        printf("      Phase: %s%d.%03d degrees\n", s, v / 1000, v % 1000);
        v = RD(base,  0x210 + (i * 12));
        printf("       Duty: %d.%03d%%\n", v / 1000, v % 1000);
    }
}

void
mmcmShow(void)
{
    mmcmDump(XPAR_RFADC_MMCM_BASEADDR);
}

static void
showAdcClk(const char *msg)
{
    uint32_t v;

    printf("%sADC clock MMCM:\n", msg);
    v = RD(XPAR_RFADC_MMCM_BASEADDR, 0x200);
    printf("     Input divider: %d\n", v & 0xFF);
    printf("        Multiplier: %d.%03d\n", (v >> 8) & 0xFF, (v >> 16) & 0x3FF);
    v = RD(XPAR_RFADC_MMCM_BASEADDR, 0x208);
    printf("           Divider: %d.%03d\n", v & 0xFF, (v >> 8) & 0x3FF);
}

void
mmcmSetAdcClkMultiplier(int multiplier)
{
    int mulInt = multiplier / 1000;
    int mulFrac = multiplier % 1000;
    uint32_t v = RD(XPAR_RFADC_MMCM_BASEADDR, 0x200);

    v &= ~0x3FFFF00;
    v |= (mulFrac << 16) | (mulInt << 8);
    WR(XPAR_RFADC_MMCM_BASEADDR, 0x200, v);
}

void
mmcmSetAdcClk0Divider(int divider)
{
    int divInt = divider / 1000;
    int divFrac = divider % 1000;

    WR(XPAR_RFADC_MMCM_BASEADDR, 0x208, (divFrac << 8) | divInt);
}

void
mmcmStartReconfig(void)
{
    uint32_t then;

    then = MICROSECONDS_SINCE_BOOT();
    while (!RD(XPAR_RFADC_MMCM_BASEADDR, 0x04) & 0x1) {
        if ((MICROSECONDS_SINCE_BOOT() - then) > 1000000) {
            warn("Critical -- ADC clock MMCM unlocked");
            break;
        }
    }
    then = MICROSECONDS_SINCE_BOOT();
    WR(XPAR_RFADC_MMCM_BASEADDR, 0x25C, 3);
    while (!RD(XPAR_RFADC_MMCM_BASEADDR, 0x04) & 0x1) {
        if ((MICROSECONDS_SINCE_BOOT() - then) > 10000000) {
            warn("Critical -- ADC clock MMCM won't lock");
        }
        return;
    }
    printf("ADC MMCM locked after %d uS.\n", MICROSECONDS_SINCE_BOOT() - then);
}

void
mmcmInit(void)
{
    showAdcClk("Old ");
    mmcmSetAdcClkMultiplier(ADC_CLK_MMCM_MULTIPLIER);
    mmcmSetAdcClk0Divider(ADC_CLK_MMCM_DIVIDER);
    mmcmStartReconfig();
    showAdcClk("");
}
