/*
 * This code is based on the suggestions provided in Xilinx answer
 * record 68785, "Manual Eye Scan with UltraScale+ GTY" and answer
 * record 66517, Manual Eye Scan with UltraScale GTY in 10 steps".
 *
 * Assumes that line rate is <= 10 Gb/s.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "eyescan.h"
#include "gpio.h"
#include "util.h"

#define PRESCALE_CODE   5
#define H_LAST          16
#define V_RANGE         128
#define V_STRIDE        8

#define DRP_REG_RX_WIDTH            0x003
#define DRP_REG_ES_CONTROL          0x03C
#define DRP_REG_ES_QUAL_MASK0       0x044
#define DRP_REG_ES_QUAL_MASK1       0x045
#define DRP_REG_ES_QUAL_MASK2       0x046
#define DRP_REG_ES_QUAL_MASK3       0x047
#define DRP_REG_ES_QUAL_MASK4       0x048
#define DRP_REG_ES_SDATA_MASK0      0x049
#define DRP_REG_ES_SDATA_MASK1      0x04A
#define DRP_REG_ES_SDATA_MASK2      0x04B
#define DRP_REG_ES_SDATA_MASK3      0x04C
#define DRP_REG_ES_SDATA_MASK4      0x04D
#define DRP_REG_ES_HORZ_OFFSET      0x04F
#define DRP_REG_RX_CONFIG           0x063
#define DRP_REG_RX_SLIDE_CONFIG     0x064
#define DRP_REG_RX_BUF_CONFIG       0x065
#define DRP_REG_ES_PHASE_CONFIG     0x094
#define DRP_REG_ES_VERT_CONTROL     0x097
#define DRP_REG_ES_QUAL_MASK5       0x0EC
#define DRP_REG_ES_QUAL_MASK6       0x0ED
#define DRP_REG_ES_QUAL_MASK7       0x0EE
#define DRP_REG_ES_QUAL_MASK8       0x0EF
#define DRP_REG_ES_QUAL_MASK9       0x0F0
#define DRP_REG_ES_SDATA_MASK5      0x0F1
#define DRP_REG_ES_SDATA_MASK6      0x0F2
#define DRP_REG_ES_SDATA_MASK7      0x0F3
#define DRP_REG_ES_SDATA_MASK8      0x0F4
#define DRP_REG_ES_SDATA_MASK9      0x0F5
#define DRP_REG_ES_ERROR_COUNT      0x251
#define DRP_REG_ES_SAMPLE_COUNT     0x252
#define DRP_REG_ES_STATUS           0x253

#define ES_CONTROL_PRESCALE_MASK    0x1F
#define ES_CONTROL_EYE_SCAN_ENABLE  0x100
#define ES_CONTROL_ERRDET_ENABLE    0x200
#define ES_CONTROL_RUN              0x400

#define ES_STATUS_DONE              0x01
#define ES_STATUS_STATE_MASK        0x0E

#define RX_CONFIG_RX_DIV_MASK       0x7

static const char *laneNames[] = EYESCAN_LANE_NAMES;
#define EYESCAN_LANECOUNT (sizeof laneNames / sizeof laneNames[0])

/*
 * ES2 and production silicon differences
 */
#define ZYNQMP_CSU_BASEADDR         0xFFCA0000
#define ZYNQMP_CSU_IDCODE_OFFSET    0x40
#define ES_PHASE_CONFIG_USE_PCS_CLK_PHASE_SEL  0x400
static struct hOffsetModifier {
    uint16_t mask;
    uint16_t augment;
} hOffsetModifiers[EYESCAN_LANECOUNT];

static enum eyescanFormat {
    FMT_ASCII_ART,
    FMT_NUMERIC,
    FMT_RAW
} eyescanFormat;

static void
drp_eyescan_reset(int lane, int reset)
{
    GPIO_WRITE(GPIO_IDX_EVR_GTY_DRP+lane, (1UL << 30) | (reset != 0));
}

static void
drp_write(int lane, int regOffset, int value)
{
    int pass = 0;
    GPIO_WRITE(GPIO_IDX_EVR_GTY_DRP+lane,
                            (1UL << 31) | (regOffset << 16) | (value & 0xFFFF));
    if (debugFlags & DEBUGFLAG_DRP) {
        printf("%x:%04x <- %04X\n", lane, regOffset, value);
    }
    while(GPIO_READ(GPIO_IDX_EVR_GTY_DRP+lane) & (1UL << 31)) {
        if (++pass == 10) {
            printf("Lane %d, reg 0x%x drp_write failed.\n", lane, regOffset);
            return;
        }
    }
}

static int
drp_read(int lane, int regOffset)
{
    int pass = 0;
    uint32_t drp;

    GPIO_WRITE(GPIO_IDX_EVR_GTY_DRP+lane, regOffset << 16);
    while((drp = GPIO_READ(GPIO_IDX_EVR_GTY_DRP+lane)) & (1UL << 31)) {
        if (++pass == 10) {
            printf("Lane %d, reg 0x%x drp_read failed.\n", lane, regOffset);
            return 0;
        }
    }
    if (debugFlags & DEBUGFLAG_DRP) {
        printf("%x:%04x -> %04X\n", lane, regOffset, drp & 0xFFFF);
    }
    return drp & 0xFFFF;
}

static void
drp_rmw(int lane, int regOffset, int mask, int value)
{
    uint16_t v = drp_read(lane, regOffset);
    v &= ~mask;
    v |= value & mask;
    drp_write(lane, regOffset, v);
}

static void
drp_set(int lane, int regOffset, int bits)
{
    drp_rmw(lane, regOffset, bits, bits);
}

static void
drp_clr(int lane, int regOffset, int bits)
{
    drp_rmw(lane, regOffset, bits, 0);
}

static void
drp_showReg(int lane, const char *msg)
{
    int i, r;
    int c = 0;
    static const short base[2] = { 0x000, 0x250 };
    static const short last[2] = { 0x116, 0x28C };

    printf("\nLANE %d DRP REGISTERS (%s):\n", lane, msg);
    for (i = 0 ; i < sizeof base / sizeof base[0] ; i++) {
        for (r = base[i] ; r <= last[i] ; r++) {
            printf("%s%03X:%04X", c ? "    " : "", r, drp_read(lane, r));
            if (++c == 6) {
                printf("\n");
                c = 0;
            }
        }
    }
    if (c) {
        printf("\n");
    }
}

/*
 * Eye scan initialization requires a subsequent PMA reset.
 * Ensure that eyescanInit() is called before gytInit().
 */
void
eyescanInit(void)
{
    int lane;
    uint32_t idCode = Xil_In32(ZYNQMP_CSU_BASEADDR+ZYNQMP_CSU_IDCODE_OFFSET);
    int deviceRevision = idCode >> 28;
    int isProductionSilicon = (deviceRevision >= 3);
    uint16_t esPhaseChangeConfig;

    printf("Device revision %d.  %s eye scan code.\n", deviceRevision,
            isProductionSilicon ? "Production silicon" : "Engineering sample");
    for (lane = 0 ; lane < EYESCAN_LANECOUNT ; lane++) {
        /* Enable statistical eye scan */
        drp_write(lane, DRP_REG_ES_CONTROL, ES_CONTROL_ERRDET_ENABLE |
                                            ES_CONTROL_EYE_SCAN_ENABLE |
                                            PRESCALE_CODE);

        /* Set ES_SDATA_MASK to check configured N bit data */
        drp_write(lane, DRP_REG_ES_SDATA_MASK0, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK1, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK2, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK3, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK4, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK5, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK6, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK7, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK8, 0xFFFF);
        drp_write(lane, DRP_REG_ES_SDATA_MASK9, 0xFFFF);
        switch (drp_read(lane, DRP_REG_RX_WIDTH) >> 5 & 0x7) {
        case 3: // 20 bit
            drp_write(lane, DRP_REG_ES_SDATA_MASK3, 0x0FFF);
            drp_write(lane, DRP_REG_ES_SDATA_MASK4, 0x0000);
            break;

        case 5: // 40 bit
            drp_write(lane, DRP_REG_ES_SDATA_MASK2, 0x00FF);
            drp_write(lane, DRP_REG_ES_SDATA_MASK3, 0x0000);
            drp_write(lane, DRP_REG_ES_SDATA_MASK4, 0x0000);
            break;
        }

        /* Enable all bits in ES_QUAL_MASK (count all bits) */
        drp_write(lane, DRP_REG_ES_QUAL_MASK0, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK1, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK2, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK3, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK4, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK5, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK6, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK7, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK8, 0xFFFF);
        drp_write(lane, DRP_REG_ES_QUAL_MASK9, 0xFFFF);

        if (isProductionSilicon) {
            hOffsetModifiers[lane].mask = 0x7FF;
            if (laneNames[lane][0] == '>') {
                /* > 10 Gb/s */
                esPhaseChangeConfig = 0;
                hOffsetModifiers[lane].augment = 0x800;
            }
            else {
                esPhaseChangeConfig = ES_PHASE_CONFIG_USE_PCS_CLK_PHASE_SEL;
                hOffsetModifiers[lane].augment = 0;
            }
        }
        else {
            esPhaseChangeConfig = 0;
            hOffsetModifiers[lane].mask = 0xFFF;
            hOffsetModifiers[lane].augment = 0;
        }
        drp_rmw(lane, DRP_REG_ES_PHASE_CONFIG,
                    ES_PHASE_CONFIG_USE_PCS_CLK_PHASE_SEL, esPhaseChangeConfig);
    }
}

/*
 * 2 characters for each point except the first.
 * (-H_LAST to -1, 0 +1 to +H_LAST) points.
 */
#define PLOT_WIDTH ((((2 * H_LAST) + 1) * 2) - 1)
static int
xBorderCrank(int lane)
{
    const int titleOffset = 2;
    static int i = PLOT_WIDTH + 1, nameIndex, laneIndex;
    char c;

    if (eyescanFormat == FMT_RAW) return 0;
    if (lane >= 0) {
        i = -1;
        nameIndex = 0;
        laneIndex = lane;
    }
    if (i <= PLOT_WIDTH) {
        if (i == -1) {
            printf("+");
        }
        else if (i == PLOT_WIDTH) {
                printf("+\n");
        }
        else {
            if (i == PLOT_WIDTH / 2) {
                c = '+';
            }
            else if ((laneIndex < EYESCAN_LANECOUNT)
                  && (i >= titleOffset)
                  && (laneNames[laneIndex][nameIndex] != '\0')) {
                c = laneNames[laneIndex][nameIndex++];
            }
            else {
                c = '-';
            }
            printf("%c", c);
        }
        i++;
    }
    return (i <= PLOT_WIDTH);
}

/*
 * Map log2err(errorCount) onto grey scale or alphanumberic value.
 * Special cases for 0 which is the only value that maps to ' ' and for
 * full scale error (131070) which is the only value that maps to '@'.
 */
static int
plotChar(int errorCount)
{
    int i, c;

    if (errorCount <= 0)          c = ' ';
    else if (errorCount >= 65535) c = '@';
    else {
        int log2err = 31 - __builtin_clz(errorCount);
        if (eyescanFormat == FMT_NUMERIC) {
            c = (log2err <= 9) ? '0' + log2err : 'A' - 10 + log2err;
        }
        else {
            i = 1 + (log2err / 2);
            c = " .:-=?*#%@"[i];
        }
    }
    return c;
}

/*
 * Perform an acquisition at 0 offset to see if synchronization is right
 */
static int
eyescanSync(int lane)
{
    int pass;
    uint32_t whenStarted;

    drp_rmw(lane, DRP_REG_ES_VERT_CONTROL, 0x7FC, 0);
    drp_rmw(lane, DRP_REG_ES_HORZ_OFFSET, 0xFFF0, 0);
    for (pass = 0 ; pass < 10 ; pass++) {
        whenStarted = MICROSECONDS_SINCE_BOOT();
        drp_set(lane, DRP_REG_ES_CONTROL, ES_CONTROL_RUN);
        for (;;) {
            if (drp_read(lane, DRP_REG_ES_STATUS) & ES_STATUS_DONE) {
                if (drp_read(lane, DRP_REG_ES_ERROR_COUNT) < 65535) {
                if (pass) {
                    printf("Lane %d synchronized after pass %d.\n", lane, pass);
                }
                    return 1;
                }
                break;
            }
            if ((int32_t)(MICROSECONDS_SINCE_BOOT() - whenStarted) > 500000) {
                drp_clr(lane, DRP_REG_ES_CONTROL, ES_CONTROL_RUN);
                return 0;
            }
        }
        /*
         * Attempt resynchronization as described in AR#68785
         */
        drp_rmw(lane, DRP_REG_ES_HORZ_OFFSET, 0xFFF0, 0x880 << 4);
        microsecondSpin(1);
        drp_eyescan_reset(lane, 1);
        drp_rmw(lane, DRP_REG_ES_HORZ_OFFSET, 0xFFF0, 0x800 << 4);
        drp_eyescan_reset(lane, 0);
    }
    printf("Lane %d eye scan did not synchronize.\n", lane);
    return 0;
}

static int
eyescanStep(int lane)
{
    static int eyescanActive, eyescanAcquiring, eyescanLane;
    static int hRange, hStride;
    static int hOffset, vOffset;
    static int errorCount;
    static int activeLane;
    static uint32_t whenStarted;

    if ((lane >= 0) && !eyescanActive) {
        /* Want H_LAST horizontal points on either side of baseline */
        int rxDiv = 1 << (drp_read(lane, DRP_REG_RX_CONFIG) & 0x7);
        hRange = 32 * rxDiv; 
        hStride = hRange / H_LAST;
        hOffset = -hRange;
        vOffset = V_RANGE;
        if (!eyescanSync(lane)) {
            return 0;
        }
        xBorderCrank(lane);
        eyescanLane = lane;
        eyescanAcquiring = 0;
        eyescanActive = 1;
        activeLane = lane;
        return 1;
    }
    if (xBorderCrank(-1)) {
        return 1;
    }
    if (eyescanActive) {
        if (eyescanAcquiring) {
            int status = drp_read(activeLane, DRP_REG_ES_STATUS);
            if ((status & ES_STATUS_DONE)
             || ((int32_t)(MICROSECONDS_SINCE_BOOT() - whenStarted) > 500000)) {
                eyescanAcquiring = 0;
                if (status & ES_STATUS_DONE) {
                    char border = vOffset == 0 ? '+' : '|';
                    errorCount = drp_read(activeLane, DRP_REG_ES_ERROR_COUNT);
                    if (eyescanFormat == FMT_RAW) {
                        int h;
                        int v;
                        h = drp_read(activeLane, DRP_REG_ES_HORZ_OFFSET);
                        h = (h >> 4) & 0x7FF;
                        if (h & 0x400) h -= 0x800;
                        v = drp_read(activeLane, DRP_REG_ES_VERT_CONTROL);
                        v = ((v & (1 << 10)) ? -1 : 1) * ((v >> 2) & 0x7F);
                        printf("%4d %4d %6d\n", h, v, errorCount);
                    }
                    else {
                        char c;
                        printf("%c", (hOffset == -hRange) ? border : ' ');
                        if ((errorCount==0) && (hOffset==0) && (vOffset==0))
                            c = '+';
                        else
                            c = plotChar(errorCount);
                        printf("%c", c);
                    }
                    hOffset += hStride;
                    if (hOffset > hRange) {
                        if (eyescanFormat != FMT_RAW) {
                            printf("%c\n", border);
                        }
                        hOffset = -hRange;
                        vOffset -= V_STRIDE;
                    }
                }
                else {
                    drp_showReg(activeLane, "SCAN FAILURE");
                    eyescanActive = 0;
                }
                drp_clr(activeLane, DRP_REG_ES_CONTROL, ES_CONTROL_RUN);
            }
            else if (debugFlags & DEBUGFLAG_DRP) {
                microsecondSpin(5000);
            }
        }
        else if (vOffset < -V_RANGE) {
            xBorderCrank(eyescanLane);
            eyescanActive = 0;
        }
        else {
            int vAbs = abs(vOffset);
            struct hOffsetModifier *hp = &hOffsetModifiers[activeLane];
            if (vAbs > 127) vAbs = 127;
            drp_rmw(activeLane, DRP_REG_ES_VERT_CONTROL, 0x7FC,
                                   (vOffset < 0 ? (1 << 10) : 0) | (vAbs << 2));
            drp_rmw(activeLane, DRP_REG_ES_HORZ_OFFSET, 0xFFF0,
                                     ((hOffset & hp->mask) | hp->augment) << 4);
            drp_set(activeLane, DRP_REG_ES_CONTROL, ES_CONTROL_RUN);
            whenStarted = MICROSECONDS_SINCE_BOOT();
            eyescanAcquiring = 1;
        }
    }
    return eyescanActive;
}

int
eyescanCrank(void)
{
    return eyescanStep(-1);
}

int
eyescanCommand(int argc, char **argv)
{
    int i;
    char *endp;
    enum eyescanFormat format = FMT_ASCII_ART;
    int lane = -1;

    if (argc == 0) {
        for (i = 0 ; i < EYESCAN_LANECOUNT ; i++) {
            drp_showReg(i, "CMD");
        }
        return 0;
    }
    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'n': format = FMT_NUMERIC;   break;
            case 'r': format = FMT_RAW;       break;
            default: return 1;
            }
            if (argv[i][2] != '\0') return 1;
        }
        else {
            lane = strtol(argv[i], &endp, 0);
            if ((*endp != '\0') || (lane < 0) || (lane >= EYESCAN_LANECOUNT))
                return 1;
        }
    }
    if (lane < 0) lane = 0;
    eyescanFormat = format;
    eyescanStep(lane);
    return 0;
}
