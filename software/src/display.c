#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "afe.h"
#include "display.h"
#include "evr.h"
#include "gpio.h"
#include "interlock.h"
#include "st7789v.h"
#include "sysmon.h"
#include "systemParameters.h"
#include "util.h"

/*
 * Special display modes for internal use
 */
#define DISPLAY_MODE_UPDATE -1
#define DISPLAY_MODE_FETCH  -2

static void
drawHeartbeatIndicator(void)
{
    int state = GPIO_READ(GPIO_IDX_USER_GPIO_CSR) & USER_GPIO_CSR_HEARTBEAT;
    static int oldState = -1;
    static const uint16_t heart[16][18] = {
    { 0x0000,0x0100,0x0580,0x0F41,0x0FC1,0x0F81,0x0600,0x0280,0x0000,0x0000,
      0x0280,0x0600,0x0F81,0x0FC1,0x0F41,0x0580,0x0100,0x0000 },
    { 0x0140,0x0EC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0F81,0x0380,0x0380,
      0x0F81,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0EC1,0x0140 },
    { 0x0580,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0F41,0x0F41,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0580 },
    { 0x0F41,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0F41 },
    { 0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1 },
    { 0x0F81,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0F81 },
    { 0x0F01,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0F01 },
    { 0x0580,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0580 },
    { 0x0200,0x0F81,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0F81,0x0200 },
    { 0x0000,0x0400,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0400,0x0000 },
    { 0x0000,0x0000,0x0440,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0440,0x0000,0x0000 },
    { 0x0000,0x0000,0x0000,0x03C0,0x0F81,0x0FC1,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0FC1,0x0F81,0x03C0,0x0000,0x0000,0x0000 },
    { 0x0000,0x0000,0x0000,0x0000,0x0300,0x0F41,0x0FC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0FC1,0x0F41,0x0300,0x0000,0x0000,0x0000,0x0000 },
    { 0x0000,0x0000,0x0000,0x0000,0x0000,0x01C0,0x0EC1,0x0FC1,0x0FC1,0x0FC1,
      0x0FC1,0x0EC1,0x01C0,0x0000,0x0000,0x0000,0x0000,0x0000 },
    { 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x00C0,0x0E41,0x0FC1,0x0FC1,
      0x0E41,0x00C0,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000 },
    { 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0040,0x05C0,0x05C0,
      0x0040,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000 },
    };
    const int rows = sizeof heart / sizeof heart[0];
    const int cols = sizeof heart[0] / sizeof heart[0][0];

    if (state != oldState) {
        const int xs = DISPLAY_WIDTH - cols;
        const int ys = DISPLAY_HEIGHT - rows;
        oldState = state;
        if (state) {
            st7789vDrawRectangle(xs, ys, cols, rows, &heart[0][0]);
        }
        else {
            st7789vFlood(xs, ys, cols, rows, ST7789V_BLACK);
        }
    }
}

static void
drawInterlockIndicator(int force)
{
    int state = interlockRelayState();
    static int oldState = -1;
    static const uint16_t open[16][23] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0800, 0x6800,
      0x6800, 0x0800, 0x0000, 0x2000, 0x8000, 0x4000, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb000,
      0xe000, 0x3000, 0x0000, 0x3800, 0xe000, 0xa800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x3000, 0xc000,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf800, 0xd000, 0x3800, 0x2800, 0x2800,
      0x2800, 0x2800, 0x2800, 0x2800, 0x2800 },
    { 0xd800, 0xd800, 0xd800, 0xd800, 0xd800, 0xd800, 0xd800, 0xd800, 0xf800,
      0xf800, 0x3800, 0x0000, 0x3800, 0xf800, 0xf800, 0xe000, 0xd800, 0xd800,
      0xd800, 0xd800, 0xd800, 0xd800, 0xd800 },
    { 0xd800, 0xd800, 0xd800, 0xd800, 0xd800, 0xd800, 0xd800, 0xe000, 0xf800,
      0xf800, 0x3800, 0x0000, 0x3800, 0xf800, 0xf800, 0xd800, 0xd800, 0xd800,
      0xd800, 0xd800, 0xd800, 0xd800, 0xd800 },
    { 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x2800, 0x3800, 0xd000,
      0xf800, 0x3800, 0x0000, 0x3800, 0xf000, 0xc000, 0x3000, 0x2800, 0x2800,
      0x2800, 0x2800, 0x2800, 0x2800, 0x2800 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xb800,
      0xf000, 0x3800, 0x0000, 0x3800, 0xf000, 0xb800, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1000, 0xa800,
      0xe000, 0x3800, 0x0000, 0x3000, 0xe000, 0xb000, 0x1000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4000,
      0x8000, 0x2000, 0x0000, 0x0800, 0x6800, 0x6800, 0x0800, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    };
    static const uint16_t closed[16][23] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0160,
      0x0160, 0x0020, 0x0000, 0x0080, 0x01a0, 0x00e0, 0x0000, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0260,
      0x02e0, 0x00a0, 0x0000, 0x00c0, 0x0300, 0x0240, 0x0020, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0020, 0x00c0, 0x00e0, 0x0040, 0x0000, 0x0040, 0x0280,
      0x0340, 0x00c0, 0x0000, 0x00c0, 0x0340, 0x0280, 0x0040, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0020, 0x01a0, 0x02e0, 0x01e0, 0x0040, 0x0040, 0x0280,
      0x0340, 0x00c0, 0x0000, 0x00c0, 0x0340, 0x0280, 0x0040, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x00a0, 0x0280, 0x0380, 0x02e0, 0x0180, 0x0280,
      0x0340, 0x00c0, 0x0000, 0x00c0, 0x0340, 0x0280, 0x0040, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0080, 0x01c0, 0x0340, 0x0380, 0x0360,
      0x0340, 0x00c0, 0x0000, 0x00c0, 0x0340, 0x0280, 0x0040, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0100, 0x02a0, 0x03e0,
      0x03e0, 0x01e0, 0x0040, 0x00c0, 0x0340, 0x02c0, 0x00c0, 0x0080, 0x0080,
      0x0080, 0x0080, 0x0080, 0x0080, 0x0080 },
    { 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x0300, 0x03e0,
      0x03e0, 0x0380, 0x0220, 0x0120, 0x0360, 0x03e0, 0x0300, 0x02e0, 0x02e0,
      0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0 },
    { 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x0300, 0x03e0,
      0x0380, 0x0280, 0x0380, 0x0360, 0x03c0, 0x0380, 0x02e0, 0x02e0, 0x02e0,
      0x02e0, 0x02e0, 0x02e0, 0x02e0, 0x02e0 },
    { 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x0080, 0x00c0, 0x02c0,
      0x0340, 0x00e0, 0x01a0, 0x0320, 0x03e0, 0x0340, 0x0120, 0x0080, 0x0080,
      0x0080, 0x0080, 0x0080, 0x0080, 0x0080 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0280,
      0x0340, 0x00c0, 0x0000, 0x0120, 0x03a0, 0x03e0, 0x0340, 0x01e0, 0x0060,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0280,
      0x0340, 0x00c0, 0x0000, 0x00c0, 0x0340, 0x0320, 0x0300, 0x0380, 0x0260,
      0x00e0, 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0280,
      0x0340, 0x00c0, 0x0000, 0x00c0, 0x0340, 0x0280, 0x00a0, 0x01e0, 0x0360,
      0x0320, 0x0120, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0280,
      0x0340, 0x00c0, 0x0000, 0x00c0, 0x0340, 0x0280, 0x0040, 0x0020, 0x0160,
      0x0260, 0x0180, 0x0040, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x0240,
      0x0300, 0x00c0, 0x0000, 0x00a0, 0x02e0, 0x0260, 0x0040, 0x0000, 0x0000,
      0x0020, 0x0040, 0x0000, 0x0000, 0x0000 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e0,
      0x01a0, 0x0080, 0x0000, 0x0020, 0x0160, 0x0160, 0x0020, 0x0000, 0x0000,
      0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
    };
    const int rows = sizeof open / sizeof open[0];
    const int cols = sizeof open[0] / sizeof open[0][0];
    const int xs = DISPLAY_WIDTH - cols - 19;
    const int ys = DISPLAY_HEIGHT - rows;

    if (force || (state != oldState)) {
        switch(state) {
        case INTERLOCK_STATE_RELAY_OPEN:
            st7789vDrawRectangle(xs, ys, cols, rows, &open[0][0]);
            break;
        case INTERLOCK_STATE_RELAY_CLOSED:
            st7789vDrawRectangle(xs, ys, cols, rows, &closed[0][0]);
            break;
        case INTERLOCK_STATE_FAULT_NOT_OPEN:
            st7789vFlood(xs, ys, cols, rows, ST7789V_GREEN);
            break;
        case INTERLOCK_STATE_FAULT_NOT_CLOSED:
            st7789vFlood(xs, ys, cols, rows, ST7789V_RED);
            break;
        case INTERLOCK_STATE_FAULT_BOTH_OFF:
            st7789vFlood(xs, ys, cols, rows, ST7789V_BLACK);
            break;
        case INTERLOCK_STATE_FAULT_BOTH_ON:
            st7789vFlood(xs, ys, cols, rows, ST7789V_ORANGE);
            break;
        default:
            st7789vFlood(xs, ys, cols, rows, ST7789V_WHITE);
            break;
        }
        oldState = state;
    }
}

static void
drawTime(int redrawAll)
{
    evrTimestamp now;
    int isBad = 0;
    static time_t then = -1;
    static char oldStr[32];
    static int wasBad;

    if ((GPIO_READ(GPIO_IDX_EVR_SYNC_CSR) & EVR_SYNC_CSR_PPS_GOOD) == 0) {
        isBad = 1;
    }
    evrCurrentTime(&now);
    if (now.secPastEpoch == 0) {
        isBad = 1;
    }
    if (isBad != wasBad) {
        wasBad = isBad;
        redrawAll = 1;
    }
    if (redrawAll || (now.secPastEpoch != then)) {
        int first = 1;
        int i;
        char str[32];
        struct tm tm;
        then = now.secPastEpoch;
        if (!isBad) {
            strftime(str, sizeof str, "%F %T", gmtime_r(&then, &tm));
        }
        for (i = 0 ; str[i] ; i++) {
            if (redrawAll || (str[i] != oldStr[i])) {
                oldStr[i] = str[i];
                if (first) {
                    if (isBad) {
                        st7789vSetCharacterRGB(ST7789V_BLACK, ST7789V_RED);
                    }
                    else {
                        st7789vSetCharacterRGB(ST7789V_WHITE, ST7789V_BLACK);
                    }
                    first = 0;
                }
                st7789vDrawChar(i * st7789vCharWidth,
                                  DISPLAY_HEIGHT - st7789vCharHeight, str[i]);
            }
        }
    }
}

void
drawIPv4Address(const void *ipv4address, int isRecoveryMode)
{
    int foreground = ST7789V_WHITE, background = ST7789V_BLACK;
    static int recovery;
    static char cbuf[20];
    if (ipv4address) {
        strncpy(cbuf, formatIP(ipv4address), sizeof(cbuf) - 1);
        recovery = isRecoveryMode;
    };
    if (recovery) {
        int t = foreground;
        foreground = background;
        background = t;
    }
    st7789vShowText(0, DISPLAY_HEIGHT - (2 * st7789vCharHeight),
                    DISPLAY_WIDTH, st7789vCharHeight,
                    foreground, background, cbuf);
}

static void
drawSerialNumbers(void)
{
    int i, l;
    char str[16];
    st7789vSetCharacterRGB(ST7789V_WHITE, ST7789V_BLACK);
    l = snprintf(str, sizeof(str) - 1, "SN:%03d/%03d",
                                       serialNumberDFE(), afeGetSerialNumber());
    for (i = 0 ; str[i] ; i++) {
        st7789vDrawChar(DISPLAY_WIDTH - ((l - i) * st7789vCharWidth),
                            DISPLAY_HEIGHT - (2 * st7789vCharHeight), str[i]);
    }
}

void
infoDraw(int redraw)
{
    static int beenHere;
    static uint32_t microsecondsThen = 0;
    static char cbuf[4][30];
    if (!beenHere) {
        beenHere = 1;
        sprintf(cbuf[0], "Information page");
        sprintf(cbuf[1], "Target: %s", __TARGET_NAME__);
        sprintf(cbuf[2], "Git hash: %8X", GPIO_READ(GPIO_IDX_GITHASH));
    }
    uint32_t microsecondsNow = GPIO_READ(GPIO_IDX_MICROSECONDS_SINCE_BOOT);
    if ((microsecondsNow - microsecondsThen > 1000000) || redraw) {
        st7789vSetCharacterRGB(ST7789V_WHITE, ST7789V_BLACK);
    for (int i=0; i<sizeof(cbuf)/sizeof(cbuf[0]); i++) {
        st7789vShowString(DISPLAY_WIDTH - st7789vCharWidth * (i==0? 22:20),
                          st7789vCharHeight + i * st7789vCharHeight,
                          cbuf[i]);
        microsecondsThen = GPIO_READ(GPIO_IDX_MICROSECONDS_SINCE_BOOT);
    }
    }
    return;
}

static int
displayRefresh(int newMode)
{
    int buttonState = (GPIO_READ(GPIO_IDX_USER_GPIO_CSR) &
                                        USER_GPIO_CSR_DISPLAY_MODE_BUTTON) != 0;
    uint32_t secondsNow = GPIO_READ(GPIO_IDX_SECONDS_SINCE_BOOT);
    uint32_t microsecondsNow = GPIO_READ(GPIO_IDX_MICROSECONDS_SINCE_BOOT);
    static enum { pageFirst,
                  pageTempInfo,
                  pagePSinfo0,
                  pagePSinfo1,
                  pageTargetInfo,
                  pageLast } newPage = pageFirst, currentPage = pageLast;
    int isPressed, newPress;
    int redrawAll = 0;
    uint32_t pressedMicroseconds, releasedSeconds;
    static int buttonOldState;
    static int displayMode = DISPLAY_MODE_STARTUP;
    static int wasPressed, isBlanked;
    static uint32_t microsecondsWhenButtonStateChanged;
    static uint32_t secondsWhenButtonStateChanged;

    if (newMode == DISPLAY_MODE_FETCH) return displayMode;
    if (buttonState != buttonOldState) {
        microsecondsWhenButtonStateChanged = microsecondsNow;
        secondsWhenButtonStateChanged = secondsNow;
        buttonOldState = buttonState;
    }
    if ((microsecondsNow - microsecondsWhenButtonStateChanged) > 20000) {
        isPressed = buttonState;
        newPress = (isPressed && !wasPressed);
        wasPressed = isPressed;
    }
    else {
        newPress = 0;
        isPressed = wasPressed;
    }
    if (isPressed) {
        pressedMicroseconds = microsecondsNow - microsecondsWhenButtonStateChanged;
        releasedSeconds = 0;
    }
    else {
        releasedSeconds = secondsNow - secondsWhenButtonStateChanged;
        pressedMicroseconds = 0;
        if (releasedSeconds > DISPLAY_ENABLE_SECONDS) {
            st7789vBacklightEnable(0);
            isBlanked = 1;
        }
    }
    if ((displayMode < 0) || (pressedMicroseconds > 1000000)) {
        newMode = DISPLAY_MODE_PAGES;
        newPage = pageFirst;
    }
    if (newPage == pageFirst) {
        newPage++;
    }
    if (newMode >= 0) {
        if (displayMode != newMode) {
            displayMode = newMode;
            currentPage = pageLast;
            redrawAll = 1;
        }
    }
    if ((displayMode == DISPLAY_MODE_PAGES) && (newPage != currentPage)) {
        currentPage = newPage;
        redrawAll = 1;
    }
    if (redrawAll) {
        st7789vFlood(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT,
               displayMode == DISPLAY_MODE_FATAL ? ST7789V_RED : ST7789V_BLACK);
    }

    if (displayMode != DISPLAY_MODE_FATAL) {
        /*
         * Common to all non-fatal display modes
         */
        drawHeartbeatIndicator();
        drawInterlockIndicator(redrawAll);
        drawTime(redrawAll);
        if (redrawAll) {
            drawIPv4Address(NULL, 0);
            drawSerialNumbers();
        }

        /*
         * Mode-specific displays
         */
        switch (displayMode) {
        case DISPLAY_MODE_PAGES:
            switch(currentPage) {
            case pagePSinfo0:    sysmonDraw(redrawAll, 0);      break;
            case pagePSinfo1:    sysmonDraw(redrawAll, 1);      break;
            case pageTempInfo:   sysmonDraw(redrawAll, 2);      break;
            case pageTargetInfo:   infoDraw(redrawAll);         break;
            default:                                            break;
            }
            break;

        case DISPLAY_MODE_WARNING:
            if (redrawAll) {
                displayShowWarning(NULL);
            }
            break;
        }
    }
    if (newPress) {
        if (isBlanked) {
            st7789vBacklightEnable(1);
            isBlanked = 0;
        }
        else {
            if (++newPage >= pageLast) {
                newPage = pageFirst;
            }
        }
    }
    return displayMode;
}

void
displayUpdate(void)
{
    displayRefresh(-1);
}

void
displaySetMode(int mode)
{
    displayRefresh(mode);
}

int
displayGetMode(void)
{
    return displayRefresh(DISPLAY_MODE_FETCH);
}

void
displayShowFatal(const char *msg)
{
    displaySetMode(DISPLAY_MODE_FATAL);
    st7789vShowText(4, 4, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4,
                                               ST7789V_BLACK, ST7789V_RED, msg);
}

void
displayShowWarning(const char *msg)
{
    int y;
    static char cbuf[400];
    static unsigned short msgs[13];
    static int charIndex, msgIndex, yNext;
    static int displayYlast, displayYsize;

    if (displayYsize == 0) {
        displayYsize = DISPLAY_HEIGHT - (2 * st7789vCharHeight);
        displayYlast = displayYsize - st7789vCharHeight;
    }
    if (msg) {
        if (displayGetMode() != DISPLAY_MODE_WARNING) {
            displaySetMode(DISPLAY_MODE_WARNING);
        }
        if ((charIndex >= sizeof(cbuf))
         || (msgIndex >= (sizeof msgs / sizeof msgs[0]))
         || (yNext > displayYlast)) {
            st7789vFlood(2, 0, DISPLAY_WIDTH - 2, displayYsize, ST7789V_YELLOW);
            charIndex = 0;
            msgIndex = 0;
            yNext = 0;
        }
        if ((charIndex < sizeof(cbuf))
         && (msgIndex < (sizeof msgs / sizeof msgs[0]))
         && (yNext <= displayYlast)) {
            int spaceRemaining = sizeof(cbuf) - charIndex;
            int len = strlen(msg) + 1;
            if (len > spaceRemaining) len = spaceRemaining;
            memcpy(&cbuf[charIndex], msg, len);
            msgs[msgIndex++] = charIndex;
            charIndex += len;
            cbuf[charIndex - 1] = '\0';
            yNext = st7789vShowText(2, yNext,
                       DISPLAY_WIDTH - 2, displayYsize - yNext,
                       ST7789V_BLACK, ST7789V_YELLOW, msg);
        }
        return;
    }
    st7789vFlood(0, 0, 2, displayYsize, ST7789V_YELLOW);
    y = 0;
    for (int m = 0 ; m < msgIndex ; m++) {
        y = st7789vShowText(2, y, DISPLAY_WIDTH - 2, displayYsize - y,
                                 ST7789V_BLACK, ST7789V_YELLOW, &cbuf[msgs[m]]);
        if (y >= yNext) break;
    }
    if (y < displayYsize)  {
        st7789vFlood(2, y, DISPLAY_WIDTH - 2, displayYsize - y, ST7789V_YELLOW);
    }
}

void
displayShowStatusLine(const char *cp)
{
    st7789vFlood(0, DISPLAY_HEIGHT - st7789vCharHeight,
                             DISPLAY_WIDTH, st7789vCharHeight, ST7789V_BLACK);
    if (cp && *cp) {
        st7789vShowText(0, DISPLAY_HEIGHT - st7789vCharHeight,
                        DISPLAY_WIDTH, st7789vCharHeight,
                        ST7789V_WHITE, ST7789V_BLACK, cp);
    }
}
