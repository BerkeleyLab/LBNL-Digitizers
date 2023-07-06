#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <xresetps_hw.h>
#include "console.h"
#include "display.h"
#include "gpio.h"
#include "st7789v.h"
#include "util.h"

int debugFlags = 0;

#define USER_GPIO_CSR_LEDS_MASK        0xFF
#define USER_GPIO_CSR_SWITCHES_MASK    0xFF00
#define USER_GPIO_CSR_SWITCHES_SHIFT   8

void
fatal(const char *fmt, ...)
{
    va_list args;
    uint32_t then = GPIO_READ(GPIO_IDX_SECONDS_SINCE_BOOT);
    char cbuf[200];

    va_start(args, fmt);
    vsnprintf(cbuf, sizeof cbuf, fmt, args);
    va_end(args);
    displayShowFatal(cbuf);
    printf("*** Fatal error: %s\n", cbuf);
    while ((GPIO_READ(GPIO_IDX_SECONDS_SINCE_BOOT) - then) < 60) {
        displayUpdate();
        checkForReset();
    }
    resetFPGA();
}

void
warn(const char *fmt, ...)
{
    va_list args;
    char cbuf[200];

    va_start(args, fmt);
    vsnprintf(cbuf, sizeof cbuf, fmt, args);
    va_end(args);
    displayShowWarning(cbuf);
    printf("*** Warning: %s\n", cbuf);
}

void
microsecondSpin(unsigned int us)
{
    uint32_t then;
    then = MICROSECONDS_SINCE_BOOT();
    while ((uint32_t)((MICROSECONDS_SINCE_BOOT()) - then) <= us) continue;
}

/*
 * Show register contents
 */
void
showReg(unsigned int i)
{
    int r;

    if (i >= GPIO_IDX_COUNT) return;
    r = GPIO_READ(i);
    printf("  R%d = %04X:%04X %11d\n", i, (r>>16)&0xFFFF, r&0xFFFF, r);
}

void
resetFPGA(void)
{
    st7789vBacklightEnable(0);
    st7789vFlood(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0);
    st7789vAwaitCompletion();
    Xil_Out32(XRESETPS_CRL_APB_RESET_CTRL, SOFT_RESET_MASK);
    printf("Reset failed!\n");
}

void
checkForReset(void)
{
    int resetIsPushed = resetRecoverySwitchPressed();
    static uint32_t whenPushed = 0;
    static int resetWasPushed = 1;
    if (resetIsPushed) {
        if (!resetWasPushed) {
            whenPushed = MICROSECONDS_SINCE_BOOT();
            if (whenPushed == 0) whenPushed = 1;
        }
        if ((whenPushed != 0)
         && ((int)(MICROSECONDS_SINCE_BOOT() - whenPushed) > 2000000)) {
            resetFPGA();
        }
    }
    resetWasPushed = resetIsPushed;
}

int
resetRecoverySwitchPressed(void)
{
    return (GPIO_READ(GPIO_IDX_USER_GPIO_CSR) &
                                      USER_GPIO_CSR_RECOVERY_MODE_BUTTON)  != 0;
}

int
serialNumberDFE(void)
{
    return GPIO_READ(GPIO_IDX_USER_GPIO_CSR) & 0xFF;
}

/*
 * Calculate the mean by using the formula:
 *
 * mean_n = mean_n-1 + (x_n - mean_n-1)/n
 */
static
float incrementalMean(int32_t *numbers, int length)
{
    int i;
    float avg = 0;

    for (i = 0; i < length; ++i) {
        avg += (numbers[i] - avg)/(float) (i + 1);
    }

    return avg;
}

float mean(int32_t *number, int length)
{
    return incrementalMean(number, length);
}
