/*
 * RF clock generation components
 */
#include <stdio.h>
#include <stdint.h>
#include "iic.h"
#include "rfadc.h"
#include "rfclk.h"
#include "util.h"

/*
 * Configure LMK04208 jitter cleaner.
 */
void
rfClkInit04208(void)
{
    int i;
    static const uint32_t lmk04208[] = {
#include "lmk04208.h"
    };
    for (i = 0 ; i < sizeof lmk04208 / sizeof lmk04208[0] ; i++) {
        lmk04208write(lmk04208[i]);
    }
}

/*
 * Initialze LMX2594 frequency synthesizer
 */
static const char * const vTuneNames[4] =
                             { "Vtune Low", "Invalid", "Locked", "Vtune High" };
static void
init2594(int muxSelect, const uint32_t *values, int n)
{
    int i;

    /*
     * Apply and remove RESET
     */
    lmx2594write(muxSelect, 0x002412);
    lmx2594write(muxSelect, 0x002410);

    /*
     * Write registers with values from TICS Pro.
     */
    for (i = 0 ; i < n ; i++) {
        uint32_t v = values[i];
        if ((v & 0xFF0000) == 0) {
            v &= ~0x4; // Force MUXOUT_LD_SEL to readback
            v |=  0x8; // Enable initial calibration
        }
        lmx2594write(muxSelect, v);
    }
}

/*
 * Start LMX2594 and verify operation
 * I do not understand why this second FCAL_EN assertion is required, but
 * the device doesn't seem to lock unless this is done.
 */
static void
start2594(int muxSelect)
{
    int vTuneCode;
    uint32_t v0 = lmx2594read(muxSelect, 0);

    /*
     * Initiate VCO calibration
     */
    lmx2594write(muxSelect, v0 | 0x8);

    /*
     * Provide some time for things to settle down
     */
    microsecondSpin(10000);

    /*
     * See if clock locked
     */
    vTuneCode = (lmx2594read(muxSelect, 110) >> 9) & 0x3;
    if (vTuneCode != 2) {
        warn("LMX2594 (SPI MUX CHAN %d) -- VCO status: %s",
                                             muxSelect, vTuneNames[vTuneCode]);
    }
}

static const uint32_t lmx2594Defaults[] = {
#include "lmx2594.h"
};

void
lmx2594Config(const uint32_t *values, int n)
{
    int i;
    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE ; i++) {
        init2594(lmx2594MuxSel[i], values, n);
    }
    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE ; i++) {
        start2594(lmx2594MuxSel[i]);
    }
}

int
lmx2594Readback(uint32_t *values, int capacity)
{
    int i;
    int n = sizeof lmx2594Defaults / sizeof lmx2594Defaults[0];
    for (i = 0 ; (i < n) && (i < capacity) ; i++) {
        int r = n - i - 1;
        int v = lmx2594read(lmx2594MuxSel[0], r);
        *values++ = (r << 16) | (v & 0xFFFF);
    }
    return n;
}

void
rfClkInit(void)
{
    static const uint32_t values[] = {
#include "lmx2594.h"
    };
    rfClkInit04208();
    lmx2594Config(values, sizeof lmx2594Defaults / sizeof lmx2594Defaults[0]);
}

void
rfClkShow(void)
{
    int i, m, r, v;
    static uint16_t chDiv[32] = {
     2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 72, 96, 128, 192, 256, 384, 512, 768};

    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE ; i++) {
        m = lmx2594MuxSel[i];
        r = lmx2594read(m, 110);
        printf("LMX2594 %c:\n", i + 'A');
        printf("       rb_VCO_SEL: %d\n", (r >> 5) & 0x7);
        v = (r >> 9) & 0x3;
        printf("      rb_LD_VTUNE: %s\n", vTuneNames[v]);
        r = lmx2594read(m, 111);
        printf("   rb_VCO_CAPCTRL: %d\n", r & 0xFF);
        r = lmx2594read(m, 112);
        printf("   rb_VCO_DACISET: %d\n", r & 0x1FF);
        r = lmx2594read(m, 9);
        printf("           OSC_2X: %d\n", (r >> 12) & 0x1);
        r = lmx2594read(m, 10);
        printf("             MULT: %d\n", (r >> 7) & 0x1F);
        r = lmx2594read(m, 11);
        printf("            PLL_R: %d\n", (r >> 4) & 0xFF);
        r = lmx2594read(m, 12);
        printf("        PLL_R_PRE: %d\n", r & 0xFFF);
        r = lmx2594read(m, 34);
        v = lmx2594read(m, 36);
        printf("            PLL_N: %d\n", ((r & 0x7) << 16) | v);
        r = lmx2594read(m, 42);
        v = lmx2594read(m, 43);
        if (r || v) {
            printf("          PLL_NUM: %d\n", (r << 16) | v);
            r = lmx2594read(m, 38);
            v = lmx2594read(m, 39);
            printf("          PLL_DEN: %d\n", (r << 16) | v);
        }
        r = (lmx2594read(m, 75) >> 6) & 0x1F;
        v = chDiv[r];
        if (v) {
            printf("            CHDIV: %d\n", v);
        }
        else {
            printf("            CHDIV: %#x\n", r);
        }
        r = lmx2594read(m, 31);
        printf("       CHDIV_DIV2: %#x\n", (r >> 14) & 0x1);
        r = lmx2594read(m, 44);
        printf("          OUTA_PD: %x\n", (r >> 6) & 0x1);
        printf("          OUTB_PD: %x\n", (r >> 7) & 0x1);
    }
}

int
lmx2594Status(void)
{
    int i;
    int v = 0;

    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE ; i++) {
        v |= ((lmx2594read(lmx2594MuxSel[i], 110) >> 9) & 0x3) << (i * 4);
    }
    return v;
}
