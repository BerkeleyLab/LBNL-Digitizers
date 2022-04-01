/*
 * Configure User MGT reference clock (SI570)
 */

#include <stdio.h>
#include <stdint.h>
#include <xiicps.h>
#include "iic.h"
#include "util.h"

static int
writeBuf(uint8_t *buf, int length)
{
    if (!iicWrite(IIC_INDEX_USER_MGT_SI570, buf, length)) {
        warn("Unable to write USER_MGT_SI570");
        return 0;
    }
    return 1;
}

static int
writeReg(int reg, int value)
{
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    return writeBuf(buf, 2);
}

int
userMGTrefClkAdjust(int offsetPPM)
{
    int i;
    uint8_t buf[6];
    double rfreq = 0;
    uint64_t rfreq_i;

    if (offsetPPM > 3500) offsetPPM = 3500;
    else if (offsetPPM < -3500) offsetPPM = -3500;
    if (!writeReg(135, 0x01)) return 0;
    if (!iicRead(IIC_INDEX_USER_MGT_SI570, 8, &buf[1], 5)) {
        warn("Unable to read USER_MGT_SI570");
        return 0;
    }
    rfreq = buf[1] & 0x3F;
    for (i = 2 ; i < 6 ; i++) {
        rfreq = (rfreq * 256.0) + buf[i];
    }
    rfreq_i = rfreq * (1.0 + (offsetPPM / 1.0e6));
    for (i = 5 ; i > 1 ; i--) {
        buf[i] = rfreq_i & 0xFF;
        rfreq_i >>= 8;
    }
    buf[1] = (buf[1] & ~0x3F) | (rfreq_i & 0x3F);
    buf[0] = 8;
    if (!writeReg(137, 0x10)) return 0;
    if (!writeBuf(buf, 6)) return 0;
    if (!writeReg(137, 0x00)) return 0;
    if (!writeReg(135, 0x40)) return 0;
    return 1;
}
