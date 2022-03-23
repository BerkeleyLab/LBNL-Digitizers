/*
 * Communicate with IIC devices
 * For now just use routines from Xilinx library.
 * At some point we might take over the I/O ourselves and not be stuck
 * here waiting for I/O to complete.
 */
#include <stdio.h>
#include <stdint.h>
#include <xiicps.h>
#include "iic.h"
#include "util.h"

const unsigned int lmx2594MuxSel[LMX2594_MUX_SEL_SIZE] = {
    SPI_MUX_2594_A_ADC,  // Tile 224, 225, 226, 227 (ADC 0, 1, 2, 3, 4, 5, 6, 7)
    SPI_MUX_2594_B_DAC,  // Tile 228, 229, 230, 231 (DAC 0, 1, 2, 3, 4, 5, 6, 7)
};

#define C0_M_IIC_ADDRESS  0x75   /* Address of controller 0 multiplexer */
#define C1_M0_IIC_ADDRESS 0x74   /* Address of controller 1 multiplexer 0 */

static int deviceIds[] = {
    XPAR_PSU_I2C_0_DEVICE_ID,
    XPAR_PSU_I2C_1_DEVICE_ID
};
#define CONTROLLER_COUNT (sizeof deviceIds/sizeof deviceIds[0])

#define MUXPORT_UNKNOWN 127
#define MUXPORT_NONE    126

struct deviceInfo {
    int16_t controllerIndex;
    uint8_t muxPort;
    uint8_t deviceAddress;
};
static struct deviceInfo deviceTable[] = {
    { 0, MUXPORT_NONE, 0x20 }, // TCA6416A Port Expander
    { 0,            0, 0x40 }, // INA226 VCCINT
    { 0,            0, 0x41 }, // INA226 VCCINT_IO_BRAM
    { 0,            0, 0x42 }, // INA226 VCC1V8
    { 0,            0, 0x43 }, // INA226 VCC1V2
    { 0,            0, 0x45 }, // INA226 VADJ_FMC
    { 0,            0, 0x46 }, // INA226 MGTAVCC
    { 0,            0, 0x47 }, // INA226 MGT1V2
    { 0,            0, 0x48 }, // INA226 MGT1V8C
    { 0,            0, 0x49 }, // INA226 VCCINT_RF
    { 0,            0, 0x4A }, // INA226 DAC_AVTT
    { 0,            0, 0x4B }, // INA226 DAC_AVCCAUX
    { 0,            0, 0x4C }, // INA226 ADC_AVCC
    { 0,            0, 0x4D }, // INA226 ADC_AVCCAUX
    { 0,            0, 0x4E }, // INA226 DAC_AVCC
    { 0,            2, 0x40 }, // IR38064 A
    { 0,            2, 0x41 }, // IR38064 B
    { 0,            2, 0x42 }, // IR38064 C
    { 0,            2, 0x46 }, // IR38064 D
    { 0,            2, 0x43 }, // IRPS5401 A
    { 0,            2, 0x44 }, // IRPS5401 B
    { 0,            2, 0x45 }, // IRPS5401 C
    { 0,            3, 0x54 }, // SYSMON
    { 1,            0, 0x54 }, // EEPROM
    { 1,            1, 0x36 }, // SI5341
    { 1,            2, 0x5D }, // USER_SI570
    { 1,            3, 0x5D }, // USER_MGT_SI570
    { 1,            4, 0x68 }, // SI5382
    { 1,            5, 0x2F }, // I2C2SPI
    { 1,            6, 0x50 }, // RFMC
    { 1,            8, 0x50 }, // FMC
    { 1,           10, 0x32 }, // SYSMON   /* Not used -- R184, R187 DNP */
    { 1,           11, 0x51 }, // SODIMM
    { 1,           12, 0x50 }, // SFP 3 INFO
    { 1,           12, 0x51 }, // SFP 3 STATUS
    { 1,           13, 0x50 }, // SFP 2 INFO
    { 1,           13, 0x51 }, // SFP 2 STATUS
    { 1,           14, 0x50 }, // SFP 1 INFO
    { 1,           14, 0x51 }, // SFP 1 STATUS
    { 1,           15, 0x50 }, // SFP 0 INFO
    { 1,           15, 0x51 }, // SFP 0 STATUS
};
#define DEVICE_COUNT (sizeof deviceTable/sizeof deviceTable[0])

struct controller {
    XIicPs Iic;
    uint8_t controllerIndex;
    uint8_t muxPort[2];
};
    static struct controller controllers[CONTROLLER_COUNT];

/*
 * Initialize IIC controllers
 */
void
initController(struct controller *cp, int deviceId)
{
	int Status;
	XIicPs_Config *Config;

    /*
     * Find and initialize the controller
     */
	Config = XIicPs_LookupConfig(deviceId);
	if (Config == NULL) fatal("XIicPs_LookupConfig");
	Status = XIicPs_CfgInitialize(&cp->Iic, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) fatal("XIicPs_CfgInitialize");
	Status = XIicPs_SelfTest(&cp->Iic);
	if (Status != XST_SUCCESS) fatal("XIicPs_SelfTest");

	/*
	 * Set the IIC serial clock rate.
	 */
	XIicPs_SetSClk(&cp->Iic, 100000);

}

void
iicInit(void)
{
    int i;
    uint8_t buf[7];

    for (i = 0 ; i < CONTROLLER_COUNT ; i++) {
        struct controller *cp = &controllers[i];
        cp->controllerIndex = i;
        initController(cp, deviceIds[i]);
        cp->muxPort[0] = i == 0 ? MUXPORT_NONE : MUXPORT_UNKNOWN;
        cp->muxPort[1] = MUXPORT_UNKNOWN;
    }

    /*
     * Configure port expander 1_2, 1_1 as outputs
     * (compatibility only. Not connected on ZCU208)
     * Configure port expander 0_1 as output and drive low -- this works
     * around a design error on the ZCU111 that resulted in the fan always
     * running at maximum speed.  The workaround has the undesirable side
     * effect of making it impossible to monitor the fan controller FANFAIL_B
     * line, but is beter than having the fan always run at full speed.
     */
    buf[0] = 0x2; /* Select output register 0 */
    buf[1] = 0x00; /* Port 0: All outputs low */
    buf[2] = 0x00; /* Port 1: All outputs low */
    buf[3] = 0x00; /* Port 0: No polarity inversion */
    buf[4] = 0x00; /* Port 1: No polarity inversion */
    buf[5] = ~0x02; /* Port 0: All inputs, except bit 1 */
    buf[6] = ~0x06; /* Port 1: All inputs except bits 2 and 1 */
    if (!iicWrite(IIC_INDEX_TCA6416A_PORT, buf, 7)) fatal("Configure TCA6416");
}

static int
iicSend(struct controller *cp, int address, const uint8_t *buf, int n)
{
    int status;

    if (debugFlags & DEBUGFLAG_IIC) {
        int i;
        printf("IIC %d:0x%02X <-", cp->controllerIndex, address);
        for (i = 0 ; i < n ; i++) printf(" %02X", buf[i]);
    }
    if (XIicPs_BusIsBusy(&cp->Iic)) {
        microsecondSpin(100);
        if (XIicPs_BusIsBusy(&cp->Iic)) {
            if (debugFlags & DEBUGFLAG_IIC) {
                printf(" == reset ==");
            }
            XIicPs_Reset(&cp->Iic);
            microsecondSpin(10000);
            if (XIicPs_BusIsBusy(&cp->Iic)) {
                printf("===== IIC %d reset failed ====\n", cp->controllerIndex);
                return 0;
            }
        }
    }
    status = XIicPs_MasterSendPolled(&cp->Iic, (uint8_t *)buf, n, address);
    if (debugFlags & DEBUGFLAG_IIC) {
        if (status != XST_SUCCESS) printf(" FAILED");
        printf("\n");
    }
    return status == XST_SUCCESS;
}

static int
iicRecv(struct controller *cp, int address, uint8_t *buf, int n)
{
    int status;

    status = XIicPs_MasterRecvPolled(&cp->Iic, buf, n, address);
    if (debugFlags & DEBUGFLAG_IIC) {
        printf("IIC %d:0x%02X ->", cp->controllerIndex, address);
        if (status == XST_SUCCESS) {
            int i;
            for (i = 0 ; i < n ; i++) printf(" %02X", buf[i]);
        }
        else {
            printf(" FAILED");
        }
        printf("\n");
    }
    return status == XST_SUCCESS;
}

/*
 * Set multiplexers
 */
static int
setMux(struct controller *cp, int muxPort)
{
    int newMux;
    int otherMux;
    uint8_t b;

    switch (cp->controllerIndex) {
    case 0:
        if ((muxPort != MUXPORT_NONE) && (muxPort != cp->muxPort[0])) {
            b = 0x4 | muxPort;
            if (iicSend(cp, C0_M_IIC_ADDRESS, &b, 1)) {
                cp->muxPort[0] = muxPort;
            }
            else {
                cp->muxPort[0] = MUXPORT_UNKNOWN;
                return 0;
            }
        }
        break;

    case 1:
        newMux = muxPort >> 3;
        otherMux = 1 - newMux;
        b = 1 << (muxPort & 0x7);
        if (cp->muxPort[otherMux] != MUXPORT_NONE) {
            uint8_t z = 0;
            if (iicSend(cp, C1_M0_IIC_ADDRESS + otherMux, &z, 1)) {
                cp->muxPort[otherMux] = MUXPORT_NONE;
            }
            else {
                cp->muxPort[otherMux] = MUXPORT_UNKNOWN;
                return 0;
            }
        }
        if (cp->muxPort[newMux] != muxPort) {
            if (iicSend(cp, C1_M0_IIC_ADDRESS + newMux, &b, 1)) {
                cp->muxPort[newMux] = muxPort;
            }
            else {
                cp->muxPort[newMux] = MUXPORT_UNKNOWN;
                return 0;
            }
        }
        break;
    }
    return 1;
}

/*
 * Read 16 bit value from PMBUS power management IC.
 * Scale by a factor of 256.
 * Why the VOUT register doesn't use the same format is a mystery to me.
 */
int
pmbusRead(unsigned int deviceIndex, unsigned int page, int reg)
{
    uint8_t ioBuf[2];
    int v;

    if ((deviceIndex >= IIC_INDEX_IRPS5401_A)
     && (deviceIndex <= IIC_INDEX_IRPS5401_C)
     && (page <= 3)) {
        static int8_t irps5401page[3] = { 0xFF, 0xFF, 0xFF };
        int i = deviceIndex - IIC_INDEX_IRPS5401_A;
        if (page != irps5401page[i]) {
            unsigned char obuf[2];
            obuf[0] = 0;
            obuf[1] = page;
            if (!iicWrite(IIC_INDEX_IRPS5401_B, obuf, 2)) {
                irps5401page[i] = 0xFF;
                return -1;
            }
            irps5401page[i] = page;
        }
     }
    if (!iicRead(deviceIndex, reg, ioBuf, 2)) {
        return -1;
    }
    v = (ioBuf[1] << 8) | ioBuf[0];
    if (reg != 0x8B) {
        int exp = (v & 0xF800) >> 11;
        v &= 0x7FF;
        if (exp & 0x10) exp -= 0x20;
        v <<= (8 + exp);
    }
    return v;
}

/*
 * Read from specified device
 */
int
iicRead(unsigned int deviceIndex, int subAddress, uint8_t *buf, int n)
{
    struct controller *cp;
    struct deviceInfo *dp;
    int deviceAddress = (deviceIndex >> 8) & 0xFF;
    deviceIndex &= 0xFF;

    if (deviceIndex >= DEVICE_COUNT) return 0;
    dp = &deviceTable[deviceIndex];
    if (deviceAddress == 0) deviceAddress = dp->deviceAddress;
    cp = &controllers[dp->controllerIndex];
    if (!setMux(cp, dp->muxPort)) return 0;
    if (subAddress >= 0) {
        int sent;
        uint8_t s = subAddress;
        XIicPs_SetOptions(&cp->Iic, XIICPS_REP_START_OPTION);
        sent = iicSend(cp, deviceAddress, &s, 1);
        XIicPs_ClearOptions(&cp->Iic, XIICPS_REP_START_OPTION);
        if (!sent) return 0;
    }
    return iicRecv(cp, deviceAddress, buf, n);
}

/*
 * Write to specified device
 */
int
iicWrite(unsigned int deviceIndex, const uint8_t *buf, int n)
{
    struct controller *cp;
    struct deviceInfo *dp;
    int deviceAddress = (deviceIndex >> 8) & 0xFF;
    deviceIndex &= 0xFF;

    if (deviceIndex >= DEVICE_COUNT) return 0;
    dp = &deviceTable[deviceIndex];
    if (deviceAddress == 0) deviceAddress = dp->deviceAddress;
    cp = &controllers[dp->controllerIndex];
    if (!setMux(cp, dp->muxPort)) return 0;
    return iicSend(cp, deviceAddress, buf, n);
}

/*
 * EEPROM I/O
 * Tricky since a single chip appears as 4 I2C devices
 * and writes must not span page boundaries.
 */
int
eepromRead(int address, void *buf, int n)
{
    struct deviceInfo *dp = &deviceTable[IIC_INDEX_EEPROM];
    struct controller *cp = &controllers[dp->controllerIndex];

    if (debugFlags & DEBUGFLAG_IIC) {
        printf("eepromRead %d@%d\n", n, address);
    }
    if (!setMux(cp, dp->muxPort)) return 0;
    while (n) {
        int devOffset = (address >> 8) & 0x3;
        uint8_t subAddress = address & 0xFF;
        int nGet = 256 - subAddress;
        if (nGet > n) nGet = n;
        if (!iicSend(cp, dp->deviceAddress + devOffset, &subAddress, 1)
         || !iicRecv(cp, dp->deviceAddress + devOffset, buf, nGet)) {
            return 0;
        }
        address += nGet;
        buf += nGet;
        n -= nGet;
    }
    return 1;
}

int
eepromWrite(int address, const void *buf, int n)
{
    struct deviceInfo *dp = &deviceTable[IIC_INDEX_EEPROM];
    struct controller *cp = &controllers[dp->controllerIndex];
    int devOffset = (address >> 8) & 0x3;
    uint8_t subAddress = address & 0xFF;
    const uint8_t *src = buf;
    int nLeft = n;

    if (!setMux(cp, dp->muxPort)) return 0;
    while (nLeft) {
        uint8_t xBuf[17];   /* One greater than page size */
        int i = 1;
        int passCount = 0;
        xBuf[0] = subAddress;
        while (nLeft) {
            xBuf[i++] = *src++;
            subAddress++;
            nLeft--;
            if ((subAddress & 0xF) == 0) break;
        }
        /* Ensure completion of write operation */
        while (!iicSend(cp, dp->deviceAddress + devOffset, xBuf, i)) {
            if (++passCount > 20) return 0;
        }
        microsecondSpin(5000);
        if (subAddress == 0) devOffset = (devOffset + 1) & 0x3;
    }
    return 1;
}

/*
 * Send n bytes to specified SPI target
 * Note that the I2C to SPI adapter chip enable lines are connected to
 * the SPI devices in reverse order to the MUX channel selection (!!!)
 */
int
spiSend(unsigned int muxSelect, const uint8_t *buf, unsigned int n)
{
    uint8_t iicBuf[20];

    if ((muxSelect >= 4) || (n >= sizeof iicBuf)) return 0;
    iicBuf[0] = 0x8 >> muxSelect;
    memcpy(&iicBuf[1], buf, n);
    if (!iicWrite(IIC_INDEX_I2C2SPI, iicBuf, n + 1)) return 0;
    return 1;
}

/*
 * Send and receive
 */
int
spiTransfer(unsigned int muxSelect, uint8_t *buf, unsigned int n)
{
    uint8_t selected = GPIO_READ(GPIO_IDX_CLK104_SPI_MUX_CSR) & 0xFF;

    /*
     * Don't exceed I2C/SPI adapter buffer limit
     */
    if (n > 200) return 0;

    /*
     * Set port expander lines 1_2, 1_1 (MISO MUX address)
     * These are the only output lines on this port of the expander
     * so we assume that the iicWrite below is the only code that
     * writes a value to port expander register 3.
     */
    /*
     * Set the SPI mux selection
     */
    if (selected != muxSelect) {
        GPIO_WRITE(GPIO_IDX_CLK104_SPI_MUX_CSR, muxSelect);
    }

    /*
     * Transmit to and receive from SPI device
     */
    if (!spiSend(muxSelect, buf, n)) return 0;

    /*
     * Read buffered data from I2C/SPI adapter
     */
    return iicRead(IIC_INDEX_I2C2SPI, -1, buf, n);
}

/*
 * LMK04828B Jitter Cleaner
 */
uint32_t
lmk04828Bread(int reg)
{
    uint8_t buf[3];

    /*
     * Select register
     */
    // Read command + W1 (0) W2 (0) + A12 - A8
    buf[0] = 0x80 | ((reg & 0x1F00) >> 8);
    // A7 - A0
    buf[1] = (reg & 0xFF);
    buf[2] = 0;
    if (!spiSend(SPI_MUX_04828B, buf, 3)) return 0;

    /*
     * Read back value
     * Transmit values are ignored, so just send the same thing.
     */
    if (!spiTransfer(SPI_MUX_04828B, buf, 3)) return 0;
    return buf[2];
}

int
lmk04828Bwrite(uint32_t value)
{
    uint8_t buf[3];

    buf[0] = value >> 16;
    buf[1] = value >> 8;
    buf[2] = value;
    if (!spiSend(SPI_MUX_04828B, buf, 3)) return 0;
    return 1;
}

/*
 * LMX2594 RF synthesizer
 */
int
lmx2594read(int muxSelect, int reg)
{
    uint8_t buf[3];

    buf[0] = 0x80 | (reg & 0x7F);
    buf[1] = 0;
    buf[2] = 0;
    if (!spiTransfer(muxSelect, buf, 3)) return 0;
    return (buf[1] << 8) | buf[2];
}

int
lmx2594write(int muxSelect, uint32_t value)
{
    uint8_t buf[3];

    buf[0] = (value >> 16) & 0x7F;
    buf[1] = value >> 8;
    buf[2] = value;
    if (!spiSend(muxSelect, buf, 3)) return 0;
    return 1;
}

int
sfpGetStatus(uint32_t *buf)
{
    uint8_t rxBuf[10];
    uint16_t temp, vcc, txPower, rxPower;

    if (iicRead (IIC_INDEX_SFP_0_STATUS, 96, rxBuf, 10)) {
        temp = (rxBuf[0] << 8) + rxBuf[1];
        vcc = (rxBuf[2] << 8) + rxBuf[3];
        txPower = (rxBuf[6] << 8) | rxBuf[7];
        rxPower = (rxBuf[8] << 8) | rxBuf[9];
        buf[0] = (vcc << 16) | temp;
        buf[1] = (rxPower << 16) | txPower;
    }
    else {
        buf[0] = 0;
        buf[1] = 0;
    }
    return 2;
}

int
sfpGetTemperature(void)
{
    uint8_t rxBuf[2];
    if (iicRead (IIC_INDEX_SFP_0_STATUS, 96, rxBuf, 2)) {
        return ((int16_t)((rxBuf[0] << 8) + rxBuf[1]) * 10) / 256;
    }
    return 2550;
}

int
sfpGetRxPower(void)
{
    uint8_t rxBuf[2];
    if (iicRead (IIC_INDEX_SFP_0_STATUS, 96+8, rxBuf, 2)) {
        return (rxBuf[0] << 8) + rxBuf[1];
    }
    return 0;
}
