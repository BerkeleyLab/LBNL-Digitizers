/*
 * Communicate with analog front end components
 */

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <xparameters.h>
#include "acquisition.h"
#include "afe.h"
#include "ffs.h"
#include "gpio.h"
#include "iic.h"
#include "rfadc.h"
#include "util.h"

#define AFE_CHANNEL_COUNT   8

#define SPI_DEVSEL_SHIFT 24
#define SPI_W_24_BIT_OP  0x80000000
#define SPI_R_BUSY       0x80000000
#define SPI_DEVSEL_MASK  0x07000000

#define SPI_WRITE(v)    GPIO_WRITE(GPIO_IDX_AFE_SPI_CSR, (v))
#define SPI_READ()      GPIO_READ(GPIO_IDX_AFE_SPI_CSR)

/*
 * Programmable gain amplifier codes
 */
#define AFE_PGA_CODE_FOR_MINIMUM_GAIN  32
#define AFE_PGA_CODE_FOR_TRAINING_GAIN 15
#define AFE_PGA_CODE_FOR_MAXIMUM_GAIN  0
#define AFE_PGA_CODE_FOR_MAX_DAC_CALIBRATION  21 // 5 dB

#define CALIBRATION_CSR_W_START         0x80000000
#define CALIBRATION_CSR_R_BUSY          0x80000000
#define CALIBRATION_CSR_ENABLE_TONE     0x40000000
#define CALIBRATION_CSR_CHANNEL_MASK    0x07000000
#define CALIBRATION_CSR_CHANNEL_SHIFT   24
#define CALIBRATION_CSR_RESULT_MASK     0xFFFF

#define GAIN_FOR_FULL_CALIBRATION  8
#define GAIN_FOR_1_4_CALIBRATION   20

#define ADC_RANGE_CSR_CMD_LATCH 0x1
#define ADC_RANGE_CSR_CMD_SHIFT 0x2

/*
 * RFMC IIC addresses (7 bit)
 */
#define PORTEX_ADDR 0x22  /* First of two, second is 0x23 */
#define DAC_ADDR    0x4C

struct afeConfig {
    int      gainCode;
    int      coupling;
    uint16_t calDAC;
    int16_t  gndReading;
    int16_t  calReading;
    int16_t  openReading;
};
static struct afeConfig afeConfig[AFE_CHANNEL_COUNT];
static uint8_t serialNumber;
static int afeMissing;

int
afeSetDAC(int value)
{
    uint8_t cbuf[3];
    int ret;
    static int oldValue;
    if (afeMissing) return 0;
    if (value < 0) value = 0;
    if (value > 65535) value = 65535;
    cbuf[0] = 0x30;
    cbuf[1] = value >> 8;
    cbuf[2] = value;
    if (!iicWrite((DAC_ADDR << 8) | IIC_INDEX_RFMC, cbuf, 3)) {
        return -1;
    }
    ret = oldValue;
    oldValue = value;
    return ret;
}

int
afeMicrovoltsToCounts(unsigned int channel, int microVolts)
{
    struct afeConfig *ap ;
    double dacVolts, countsPerVolt, fCounts;
    int offset, iCounts;

    if (channel >= CFG_ADC_PHYSICAL_COUNT) return 32767;
    ap = &afeConfig[channel];
    dacVolts = (2.5 * ap->calDAC) / 65536.0;
    countsPerVolt = (ap->calReading - ap->gndReading) / dacVolts;
    offset = (ap->coupling==AFE_COUPLING_AC) ? ap->openReading : ap->gndReading;
    fCounts = (microVolts * 1e-6 * countsPerVolt) + offset;
    if (fCounts > 32767) iCounts = 32767;
    else if (fCounts < -32768) iCounts = -32768;
    else iCounts = fCounts;
    return iCounts;
}

static void
setPGA(unsigned int channel, int gainCode)
{
    /* Determine DAC value for reasonable ADC */
    int step = AFE_PGA_CODE_FOR_MAX_DAC_CALIBRATION - gainCode;
    float gainChangePerStep = 0.891250938133746; // 10**(-1/20) (-1 dB)
    float gain = 1;
    int dac;
    while (step-- > 0) {
        gain *= gainChangePerStep;
    }
    /* DAC can drive only 3/4 of full scale into 50 ohm load */
    dac = ((65536 * 3) / 4) * gain;
    afeConfig[channel].calDAC = dac;

    while (SPI_READ() & SPI_R_BUSY) continue;
    SPI_WRITE(((channel << SPI_DEVSEL_SHIFT) & SPI_DEVSEL_MASK) | (2 << 16) |
                                                      ((gainCode & 0xFF) << 8));
}

void
afeInit(void)
{
    int i;
    uint8_t c[4];

    if (rfADClinkCouplingIsAC()) {
        printf("RF ADC AC coupled -- inhibiting AFE control/monitoring.\n");
        afeMissing = 1;
        for (i = 0 ; i < AFE_CHANNEL_COUNT ; i++) {
            // Set calibration to give [-1.0, 1.0) V range
            afeConfig[i].calDAC = (65536 / 2) / 2.5; // Counts for 0.5 V
            afeConfig[i].calReading = 16384; // 0.5 full scale
            afeConfig[i].gndReading = 0;
            afeConfig[i].openReading = 0;
        }
        return;
    }
    for (i = 0 ; i < AFE_CHANNEL_COUNT ; i++) {
        int rev = lmh6401GetRegister(i, 0);
        int id = lmh6401GetRegister(i, 1);
        if ((rev < 3) || (id != 0)) {
            warn("LMH6401 %d REV/ID = %d/%d", i, rev, id);
        }
    }

    /* Set calibration DAC to value that can drive all channels simultaneously */
    if (afeSetDAC(0) < 0) {
        warn("Can't set up AFE DAC");
    }

    /* Set port expander output registers to known value */
    for (i = 0 ; i < AFE_CHANNEL_COUNT ; i++) {
        afeSetCoupling(i, AFE_COUPLING_OPEN);
        setPGA(i, AFE_PGA_CODE_FOR_MINIMUM_GAIN);
        afeConfig[i].gainCode = AFE_PGA_CODE_FOR_MINIMUM_GAIN;
    }

    /* Set port expander direction/polarity registers */
    for (i = 0 ; i < (AFE_CHANNEL_COUNT+3)/4 ; i++) {
        int err = 0;
        c[0] = 0x88;    /* No polarity inversion */
        c[1] = 0x0;
        c[2] = 0x0;
        c[3] = 0x0;
        if (!iicWrite(((PORTEX_ADDR + i) << 8) | IIC_INDEX_RFMC, c, 4)) err = 1;
        c[0] = 0x8C;    /* Port direction */
        c[1] = 0x0;     /*    0 out */
        c[2] = 0x0;     /*    1 out */
        c[3] = 0xFF;    /*    2 in  */
        if (!iicWrite(((PORTEX_ADDR + i) << 8) | IIC_INDEX_RFMC, c, 4)) err = 1;
        if (err) warn("Can't set up port expander %d", i);
    }

    /* Get AFE serial number */
    iicRead((PORTEX_ADDR << 8) | IIC_INDEX_RFMC, 0x2, c, 1);
    serialNumber = c[0];
}

void
afeStart(void)
{
    int i;
    /* Set default coupling */
    if (afeMissing) return;
    for (i = 0 ; i < CFG_ADC_PHYSICAL_COUNT ; i++) {
        if (afeSetCoupling(i, AFE_COUPLING_OPEN) < 0) {
            warn("Can't set AFE %d coupling", i);
        }
        afeSetGain(i, AFE_PGA_CODE_FOR_MINIMUM_GAIN);
    }
}

int
afeGetSerialNumber(void)
{
    return serialNumber;
}

enum calibrationReadingType { cal_train, cal_gnd, cal_dac, cal_open };

static int
getCalibrationReading(int channel, enum calibrationReadingType type)
{
    uint32_t csr = GPIO_READ(GPIO_IDX_CALIBRATION_CSR);
    uint32_t whenStarted;
    int min, max;
    int badExtents = 0;
    uint32_t extents[AFE_CHANNEL_COUNT];

    // Take a reading
    whenStarted = MICROSECONDS_SINCE_BOOT();
    GPIO_WRITE(GPIO_IDX_ADC_RANGE_CSR, ADC_RANGE_CSR_CMD_LATCH);
    GPIO_WRITE(GPIO_IDX_CALIBRATION_CSR, CALIBRATION_CSR_W_START | csr);
    while (GPIO_READ(GPIO_IDX_CALIBRATION_CSR) & CALIBRATION_CSR_R_BUSY) {
        // Calibration time is proportional to the ADC clock frequency.
        // With a 500MHz clock, the 23 bit counter takes:
        // 2^22*(1/500*1e3)/1e3 us ~ 8388 us. So, using 10000
        // gives enough time for the calibration to finish.
        // Using 123MHZ clock:
        // 2^22*(1/123*1e3)/1e3 us ~ 34100 us. So, we need
        // to increase time limit. Using 50000 for now, but this
        // should be automatic!
        if ((MICROSECONDS_SINCE_BOOT() - whenStarted) > 50000) {
            warn("Calibration did not complete");
            break;
        }
    }

    // Check extents
    afeFetchADCextents(extents);
    min = (int16_t)(extents[channel] & 0xFFFF);
    max = (int16_t)(extents[channel] >> 16);
    switch (type) {
    case cal_train:
        if ((max < 8000) || (max > 27000) || (min < -27000) || (min > -8000)) {
            badExtents = 1;
        }
        break;

    case cal_gnd:
        if ((min < -10000) || (max > 10000) || (abs(max - min) > 5000)) {
            badExtents = 1;
        }
        break;

    default:
        if (abs(max - min) > 5000) {
            badExtents = 1;
        }
        break;
    }
    if (badExtents) {
        static int warnCount;
        if (warnCount < 20) {
            warnCount++;
            warn("C%d T:%d min:%d max:%d", channel, type, min, max);
        }
    }
    return GPIO_READ(GPIO_IDX_CALIBRATION_CSR) & CALIBRATION_CSR_RESULT_MASK;
}

static void
calibrate(int channel)
{
    int coupling = afeConfig[channel].coupling;
    int gainCode = afeConfig[channel].gainCode;
    uint32_t csrChan = channel << CALIBRATION_CSR_CHANNEL_SHIFT;
    uint32_t oldCSR = GPIO_READ(GPIO_IDX_CALIBRATION_CSR);
    int oldDAC;

    // Enable training signal
    setPGA(channel, AFE_PGA_CODE_FOR_MINIMUM_GAIN);
    afeSetCoupling(channel, AFE_COUPLING_TRAINING);
    setPGA(channel, AFE_PGA_CODE_FOR_TRAINING_GAIN);
    GPIO_WRITE(GPIO_IDX_CALIBRATION_CSR, CALIBRATION_CSR_ENABLE_TONE | csrChan);
    microsecondSpin(2000);
    // Enable ADC background calibration now that training signal is present
    rfADCfreezeCalibration(channel, 0);
    // Allow time for RF ADC Gain Calibration Block
    // and Time Skew Calibration Block to settle
    microsecondSpin(5000);
    // Verify that training signal is present and reasonable
    getCalibrationReading(channel, cal_train);
    // Disable ADC background calibration
    rfADCfreezeCalibration(channel, 1);
    // Disable training signal
    GPIO_WRITE(GPIO_IDX_CALIBRATION_CSR, csrChan);
    // Get 0V reading
    oldDAC = afeSetDAC(0);
    afeSetCoupling(channel, AFE_COUPLING_CALIBRATION);
    setPGA(channel, gainCode);
    microsecondSpin(4000);
    afeConfig[channel].gndReading = getCalibrationReading(channel, cal_gnd);
    // Get calibration voltage reading
    afeSetDAC(afeConfig[channel].calDAC);
    microsecondSpin(1000);
    afeConfig[channel].calReading = getCalibrationReading(channel, cal_dac);
    afeSetDAC(0);
    // Get open connection reading
    afeSetCoupling(channel, AFE_COUPLING_OPEN);
    microsecondSpin(1000);
    afeConfig[channel].openReading = getCalibrationReading(channel, cal_open);
    // Adjust trigger count level to reflect new calibration
    acquisitionScaleChanged(channel);
    // Restore training signal
    if (oldCSR & CALIBRATION_CSR_ENABLE_TONE) {
        GPIO_WRITE(GPIO_IDX_CALIBRATION_CSR, CALIBRATION_CSR_ENABLE_TONE | csrChan);
    }
    // Restore DAC
    afeSetDAC(oldDAC);
    // Restore coupling
    afeSetCoupling(channel, coupling);
    if (debugFlags & DEBUGFLAG_CALIBRATION) {
        float dacVolts = (2.5 * afeConfig[channel].calDAC) / 65536.0;
        float countsPerVolt =
               (afeConfig[channel].calReading - afeConfig[channel].gndReading) /
                                                                       dacVolts;
        printf("C:%d gCode:%d gnd:%d %d(%.3fV):%d open:%d count/V:%.1f\n",
                                                channel,
                                                afeConfig[channel].gainCode,
                                                afeConfig[channel].gndReading,
                                                afeConfig[channel].calDAC,
                                                dacVolts,
                                                afeConfig[channel].calReading,
                                                afeConfig[channel].openReading,
                                                countsPerVolt);
    }
}

void
afeADCrestart(void)
{
    int i;

    if (afeMissing) return;
    // Apply 0V to all ADCs
    // DAC can drive 0V to all channels simultaneously since
    // termination resistors are connected to ground.
    afeSetDAC(0);
    for (i = 0 ; i < CFG_ADC_PHYSICAL_COUNT ; i++) {
        setPGA(i, AFE_PGA_CODE_FOR_MINIMUM_GAIN);
        afeSetCoupling(i, AFE_COUPLING_CALIBRATION);
    }
    // Unfreeze all ADCs
    for (i = 0 ; i < CFG_ADC_PHYSICAL_COUNT ; i++) {
        rfADCfreezeCalibration(i, 0);
    }
    // Perform ADC restart (foreground calibration)
    rfADCrestart();
    // Perform ADC synchronization
    rfADCsync();
    // Uncouple DAC
    for (i = 0 ; i < CFG_ADC_PHYSICAL_COUNT ; i++) {
        afeSetCoupling(i, AFE_COUPLING_OPEN);
    }
    // Set gain and coupling and perform background training and calibration
    for (i = 0 ; i < CFG_ADC_PHYSICAL_COUNT ; i++) {
        calibrate(i);
    }
}

int
afeSetCoupling(unsigned int channel, int coupling)
{
    int idx = channel >> 1;
    int reg = ((channel >> 1) & 0x1) + 0x4;
    int addr = ((channel >> 2) & 0x1) + PORTEX_ADDR;
    int shift = 4 * (channel & 0x1);
    int mask = 0xF << shift;
    uint8_t c[2];
    static uint8_t shadow[(AFE_CHANNEL_COUNT+1)/2];

    if (afeMissing) return 0;
    if (channel >= AFE_CHANNEL_COUNT) return -1;
    c[0] = reg;
    c[1] = (shadow[idx] & ~mask) | (((1 << coupling) << shift) & mask);
    if (!iicWrite((addr << 8) | IIC_INDEX_RFMC, c, 2)) return -1;
    shadow[idx] = c[1];
    afeConfig[channel].coupling = coupling;
    return 0;
}

int
afeSetGain(unsigned int channel, int gainCode)
{
    if (afeMissing) return 0;
    if (channel >= CFG_ADC_PHYSICAL_COUNT) return -1;
    afeConfig[channel].gainCode = gainCode;
    calibrate(channel);
    return 0;
}

int
afeFetchCalibration(unsigned int channel, uint32_t *buf)
{
    int idx = 0;
    struct afeConfig *ap;
    if (channel >= AFE_CHANNEL_COUNT) return 0;
    ap = &afeConfig[channel];
    buf[idx++] = ap->coupling;
    buf[idx++] = (ap->openReading << 16) | (ap->gndReading & 0xFFFF);
    buf[idx++] = (ap->calReading << 16) | ap->calDAC;
    return idx;
}

void
afeEnableTrainingTone(int enable)
{
    uint32_t csr = GPIO_READ(GPIO_IDX_CALIBRATION_CSR);
    if (afeMissing) return;
    if (debugFlags & DEBUGFLAG_CALIBRATION) {
        printf("afeEnableTrainingTone(%d)\n", enable);
    }
    if (enable) {
        csr |= CALIBRATION_CSR_ENABLE_TONE;
    }
    else {
        csr &= ~CALIBRATION_CSR_ENABLE_TONE;
    }
    GPIO_WRITE(GPIO_IDX_CALIBRATION_CSR, csr);
}

int
lmh6401GetRegister(unsigned int channel, int r)
{
    uint32_t csr;
    if (afeMissing) return 0;
    if (channel >= AFE_CHANNEL_COUNT) return 0;
    while (SPI_READ() & SPI_R_BUSY) continue;
    SPI_WRITE(((channel << SPI_DEVSEL_SHIFT) & SPI_DEVSEL_MASK) |
                                                   ((0x80 | (r & 0x7F)) << 16));
    while ((csr = SPI_READ()) & SPI_R_BUSY) continue;
    return csr & 0xFF;
}

void
afeShow(void)
{
    int channel, reg;

    if (afeMissing) return;
    for (channel = 0 ; channel < AFE_CHANNEL_COUNT ; channel++) {
        printf("%2d:", channel);
        for (reg = 0 ; reg < 3 ; reg++) {
            int v = lmh6401GetRegister(channel, reg);
            printf("  %2.2X", v);
        }
        printf("\n");
    }
}

/*
 * Fetch ADC extents for each channel
 */
int
afeFetchADCextents(uint32_t *buf)
{
    int channel, i;
    GPIO_WRITE(GPIO_IDX_ADC_RANGE_CSR, ADC_RANGE_CSR_CMD_LATCH);
    for (channel = 0 ; channel < CFG_ADC_PHYSICAL_COUNT ; channel++) {
        int min = INT_MAX, max = INT_MIN;
        for (i = 0 ; i < CFG_AXI_SAMPLES_PER_CLOCK ; i++) {
            int v;
            v = (int16_t)(GPIO_READ(GPIO_IDX_ADC_RANGE_CSR) & 0xFFFF);
            GPIO_WRITE(GPIO_IDX_ADC_RANGE_CSR, ADC_RANGE_CSR_CMD_SHIFT);
            if (v < min) min = v;
            v = (int16_t)(GPIO_READ(GPIO_IDX_ADC_RANGE_CSR) & 0xFFFF);
            GPIO_WRITE(GPIO_IDX_ADC_RANGE_CSR, ADC_RANGE_CSR_CMD_SHIFT);
            if (v > max) max = v;
        }
        *buf++ = (max << 16) | (min & 0xFFFF);
    }
    return CFG_ADC_PHYSICAL_COUNT;
}

/*
 * EEPROM I/O
 */
void
afeFetchEEPROM(void)
{
    uint8_t buf[128];
    FRESULT fr;
    FIL fil;
    UINT nWritten;

    if (afeMissing) return;
    fr = f_open(&fil, "/"AFE_EEPROM_NAME, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        return;
    }
    if (iicRead((0x50 << 8) | IIC_INDEX_RFMC, 0, buf, sizeof buf)) {
        f_write(&fil, buf, sizeof buf, &nWritten);
    }
    f_close(&fil);
}

/*
 * Copy file to AFE EEPROM
 */
void
afeStashEEPROM(void)
{
    int address = 0;
    int pass;
    uint8_t buf[17];
    FRESULT fr;
    FIL fil;
    UINT nRead;

    if (afeMissing) return;
    fr = f_open(&fil, "/"AFE_EEPROM_NAME, FA_READ);
    if (fr != FR_OK) {
        return;
    }
    for (;;) {
        fr = f_read(&fil, buf+1, sizeof(buf)-1, &nRead);
        if (fr != FR_OK) {
            printf("IIC file read failed\n");
            break;
        }
        if (nRead == 0) {
            break;
        }
        pass = 0;
        buf[0] = address;
        while (!iicWrite((0x50 << 8) | IIC_INDEX_RFMC, buf, nRead + 1)) {
            if (++pass > 12) {
                printf("IIC EEPROM write failed\n");
                f_close(&fil);
                return;
            }
            microsecondSpin(500);
        }
        address += nRead;
    }
    f_close(&fil);
}
