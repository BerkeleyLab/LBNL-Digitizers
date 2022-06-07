/*
 * Copyright 2021, Lawrence Berkeley National Laboratory
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "gpio.h"
#include "mgt.h"
#include "util.h"
#include "rfadc.h"

#define CSR_W_RX_BIT_SLIDE          0x80
#define CSR_W_BB_TX_STRT_REQUEST    0x08
#define CSR_W_BB_TX_RST_REQUEST     0x04
#define CSR_RW_TX_RESET             0x02
#define CSR_RW_GT_RESET_ALL         0x01

#define CSR_R_BAD_K_MASK            0xFFC00000U
#define CSR_R_BAD_K_SHIFT           22
#define CSR_R_BAD_CHAR_MASK         0x003FF000U
#define CSR_R_BAD_CHAR_SHIFT        12
#define CSR_R_RX_ALIGNED            0x080
#define CSR_R_TX_RESET_DONE         0x040
#define CSR_R_RX_RESET_DONE         0x020
#define CSR_R_CPLL_LOCKED           0x010
#define CSR_R_BB_TX_RST_ERROR       0x08
#define CSR_R_BB_TX_RST_DONE        0x04
#define CSR_R_RESETS_DONE (CSR_R_TX_RESET_DONE | \
                           CSR_R_RX_RESET_DONE | \
                           CSR_R_CPLL_LOCKED)

/*
 * Return monitor values
 */
static uint16_t lostAlignmentCount;
static uint16_t rxSlideCount;
int
mgtFetch(uint32_t *args)
{
    int aIndex = 0;
    args[aIndex++] = (rxSlideCount << 16) | lostAlignmentCount;
    return aIndex;
}

/*
 * Receiver can place its recovered clock at 20 different phases relative to
 * the incoming data.  This is not acceptable since it affects the alignment
 * of acquired data to the RF reference clock so the receiver is configured
 * with automatic bit-slide disabled and manual bit-slide never performed.
 * Instead the following state machine keeps resetting the receiver until the
 * receiver comes out of reset in the spot that is aligned.
 */
int
mgtCrankRxAligner(void)
{
    uint32_t csr = GPIO_READ(GPIO_IDX_GTY_CSR);
    static uint32_t whenEntered;
    static int resetCount;
    static enum state { S_APPLY_RESET, S_HOLD_RESET, S_AWAIT_RESET_COMPLETION,
                        S_POST_RESET_DELAY, S_CONFIRM_ALIGNMENT,
                        S_ALIGNMENT_ACHIEVED, S_ALIGNED } state = S_ALIGNED;
    enum state oldState = state;

    switch (state) {
    case S_APPLY_RESET:
        GPIO_WRITE(GPIO_IDX_GTY_CSR, CSR_RW_GT_RESET_ALL);
        state = S_HOLD_RESET;
        break;

    case S_HOLD_RESET:
        if ((MICROSECONDS_SINCE_BOOT() - whenEntered) > 10) {
            resetCount++;
            if ((debugFlags & DEBUGFLAG_EVR) && (resetCount % 1000000) == 0) {
                printf("EVR reset count %d\n", resetCount);
            }
            GPIO_WRITE(GPIO_IDX_GTY_CSR, 0);
            state = S_AWAIT_RESET_COMPLETION;
        }
        break;

    case S_AWAIT_RESET_COMPLETION:
        /*
         * Large timeout is to limit warning message rate to a reasonable value
         * when hardware is misbehaving.  No problem since the timeout is very
         * unlikely to ever be reached.
         */
        if ((csr & CSR_R_RESETS_DONE) == CSR_R_RESETS_DONE) {
            state = S_POST_RESET_DELAY;
        }
        else if ((MICROSECONDS_SINCE_BOOT() - whenEntered) > 250000) {
            warn("EVR reset incomplete: %X", csr);
            state = S_APPLY_RESET;
        }
        break;

    case S_POST_RESET_DELAY:
        if ((MICROSECONDS_SINCE_BOOT() - whenEntered) > 200) {
            state = S_CONFIRM_ALIGNMENT;
        }

    case S_CONFIRM_ALIGNMENT:
    // FIXME: CHECK RESET DONE TOO?
        if (!(csr & CSR_R_RX_ALIGNED)) {
            state = S_APPLY_RESET;
        }
        else if ((MICROSECONDS_SINCE_BOOT() - whenEntered) > 1000) {
            state = S_ALIGNMENT_ACHIEVED;
        }
        break;

    case S_ALIGNMENT_ACHIEVED:
        printf("EVR aligned after %d resets.\n", resetCount);
        resetCount = 0;
        GPIO_WRITE(GPIO_IDX_GTY_CSR, CSR_RW_TX_RESET);
        microsecondSpin(2);
        GPIO_WRITE(GPIO_IDX_GTY_CSR, 0);
        rfADCsync();
        state = S_ALIGNED;
        break;

    case S_ALIGNED:
        if (!(csr & CSR_R_RX_ALIGNED)) {
            printf("EVR misaligned after %u us. Bad K:%d Char:%d\n",
                           MICROSECONDS_SINCE_BOOT() - whenEntered,
                           (csr & CSR_R_BAD_K_MASK) >> CSR_R_BAD_K_SHIFT,
                           (csr & CSR_R_BAD_CHAR_MASK) >> CSR_R_BAD_CHAR_SHIFT);
            lostAlignmentCount++;
            state = S_APPLY_RESET;
        }
        break;
    }
    if (state != oldState) {
        whenEntered = MICROSECONDS_SINCE_BOOT();
    }
    return (state == S_ALIGNED);
}

void
mgtInit(void)
{
    uint32_t csr, then, start;
    int pass = 0;
    GPIO_WRITE(GPIO_IDX_GTY_CSR, CSR_RW_GT_RESET_ALL);
    microsecondSpin(100);
    GPIO_WRITE(GPIO_IDX_GTY_CSR, 0);
    then = MICROSECONDS_SINCE_BOOT();
    while (((csr = GPIO_READ(GPIO_IDX_GTY_CSR)) & CSR_R_RESETS_DONE) !=
                                                            CSR_R_RESETS_DONE) {
        if ((MICROSECONDS_SINCE_BOOT() - then) > 250000) {
            warn("EVR didn't reset: %X", csr);
            return;
        }
    }
    printf("MGT CSR after reset: %X\n", GPIO_READ(GPIO_IDX_GTY_CSR));
    start = then = MICROSECONDS_SINCE_BOOT();
    while (!mgtCrankRxAligner()) {
        uint32_t now = MICROSECONDS_SINCE_BOOT();
        if ((now - then) > 2000000) {
            if (++pass >= 10) {
                warn("No EVR alignment -- CSR:%X", GPIO_READ(GPIO_IDX_GTY_CSR));
                return;
            }
            then = now;
            microsecondSpin(200000);
        }
        microsecondSpin(5);
    }
    printf("mgtInit done: %d us\n", MICROSECONDS_SINCE_BOOT()- start);
}


void
mgtRxBitslide(void)
{
    rxSlideCount++;
    GPIO_WRITE(GPIO_IDX_GTY_CSR, CSR_W_RX_BIT_SLIDE);
}
