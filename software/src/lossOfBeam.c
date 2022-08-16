/*
 * Deal with loss of beam firmware
 */

#include <stdio.h>
#include <stdint.h>
#include "gpio.h"

#define REG(base,chan)  ((base) + (GPIO_IDX_PER_BPM * (chan)))

void lossOfBeamThreshold(int bpm, uint32_t thrs)
{
    int ch;

    if (bpm < 0) {
        for (ch = 0 ; ch < CFG_BPM_COUNT ; ch++) {
            GPIO_WRITE(REG(GPIO_IDX_LOSS_OF_BEAM_THRSH, ch), thrs);
        }
    }
    else if (bpm < CFG_BPM_COUNT) {
        GPIO_WRITE(REG(GPIO_IDX_LOSS_OF_BEAM_THRSH, bpm), thrs);
    }
}
