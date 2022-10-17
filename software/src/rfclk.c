/*
 * RF clock generation components
 */
#include <stdio.h>
#include <stdint.h>
#include <xil_assert.h>
#include "iic.h"
#include "rfadc.h"
#include "rfclk.h"
#include "util.h"

/*
 * Configure LMK04208/LMK04828B jitter cleaner.
 */
void
rfClkInitLMK04xx(void)
{
    int i;
#if defined (__TARGET_HSD_ZCU111__) || defined (__TARGET_BCM_ZCU111__)
    static const uint32_t lmk04208[] = {
#include "lmk04208.h"
    };
    for (i = 0 ; i < sizeof lmk04208 / sizeof lmk04208[0] ; i++) {
        lmk04208write(lmk04208[i]);
    }
#elif defined (__TARGET_HSD_ZCU208__) || defined (__TARGET_BPM_ZCU208__)
    static const uint32_t lmk04828B[] = {
#include "lmk04828B.h"
    };
    for (i = 0 ; i < sizeof lmk04828B / sizeof lmk04828B[0] ; i++) {
        lmk04828Bwrite(lmk04828B[i]);
    }
#else
#   error "Unrecognized __TARGET_XXX__ macro"
#endif
}

/*
 * Initialze LMX2594 frequency synthesizer
 */
static const char * const vTuneNames[4] =
                             { "Vtune Low", "Invalid", "Locked", "Vtune High" };
static int lmx2594v0InitValues[LMX2594_MUX_SEL_SIZE];

static void
init2594(int muxSelect, const uint32_t *values, int n)
{
    Xil_AssertVoid(muxSelect < LMX2594_MUX_SEL_SIZE);

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

            // Don't initilize VCO calibration and force readback
            // for the default value
            lmx2594v0InitValues[muxSelect] = v & ~(0x8 | 0x4);
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
    Xil_AssertVoid(muxSelect < LMX2594_MUX_SEL_SIZE);

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

    /*
     * Configure STATUS pin to lock detect
     */
    lmx2594write(muxSelect, v0 | 0x4);
}

void
lmx2594Config(int muxSelect, const uint32_t *values, int n)
{
    Xil_AssertVoid(muxSelect < LMX2594_MUX_SEL_SIZE);

    init2594(muxSelect, values, n);
    start2594(muxSelect);
}

void
lmx2594ConfigAllSame(const uint32_t *values, int n)
{
    int i;
    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE; i++) {
        lmx2594Config(lmx2594MuxSel[i], values, n);
    }
}

int
lmx2594Readback(int muxSelect, uint32_t *values, int capacity)
{
    Xil_AssertNonvoid(muxSelect < LMX2594_MUX_SEL_SIZE);

    /*
     * Configure STATUS pin to readback
     */
    int v0 = lmx2594v0InitValues[muxSelect];
    lmx2594write(muxSelect, v0 & ~0x4);

    int i;
    int n = lmx2594Sizes[muxSelect];
    for (i = 0 ; (i < n) && (i < capacity) ; i++) {
        int r = n - i - 1;
        int v = lmx2594read(muxSelect, r);
        *values++ = (r << 16) | (v & 0xFFFF);
    }

    /*
     * Configure STATUS pin to lock detect
     */
    lmx2594write(muxSelect, v0 | 0x4);
    return n;
}

int
lmx2594ReadbackFirst(uint32_t *values, int capacity)
{
    return lmx2594Readback(lmx2594MuxSel[0], values, capacity);
}

void
rfClkInit(void)
{
    rfClkInitLMK04xx();
    int i;
    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE ; i++) {
        lmx2594Config(lmx2594MuxSel[i], lmx2594Values[i],
                lmx2594Sizes[i]);
    }
}

void
rfClkShow(void)
{
    int i, m, r, v;
    static uint16_t chDiv[32] = {
     2, 4, 6, 8, 12, 16, 24, 32, 48, 64, 72, 96, 128, 192, 256, 384, 512, 768};

    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE ; i++) {
        m = lmx2594MuxSel[i];

        /*
         * Configure STATUS pin to readback
         */
        int v0 = lmx2594v0InitValues[m];
        lmx2594write(m, v0 & ~0x4);

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

        /*
         * Configure STATUS pin to lock detect
         */
        lmx2594write(m, v0 | 0x4);
    }
}

int
lmx2594Status(void)
{
    int i, m;
    int v = 0;

    for (i = 0 ; i < LMX2594_MUX_SEL_SIZE ; i++) {
        m = lmx2594MuxSel[i];
        /*
         * Configure STATUS pin to readback
         */
        int v0 = lmx2594v0InitValues[m];
        lmx2594write(m, v0 & ~0x4);

        v |= ((lmx2594read(m, 110) >> 9) & 0x3) << (i * 4);

        /*
         * Configure STATUS pin to lock detect
         */
        lmx2594write(m, v0 | 0x4);
    }
    return v;
}
