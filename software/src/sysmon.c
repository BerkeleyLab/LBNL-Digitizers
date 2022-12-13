/*
 * System monitoring
 *  PS SYSMON
 *  PL SYSMON
 *  I2C devices
 */
#include <stdio.h>
#include <stdint.h>
#include <xsysmonpsu.h>
#include "acquisition.h"
#include "display.h"
#include "evr.h"
#include "epics.h"
#include "frequencyMonitor.h"
#include "gpio.h"
#include "iic.h"
#include "rfadc.h"
#include "st7789v.h"
#include "sysmon.h"
#include "util.h"

static struct {
    int     iicIndex;
    int     ampsPerVolt; /* 1/Rshunt */
    const char *name;
} const psInfo[] = {
    { IIC_INDEX_INA226_VCCINT,         500, "VccINT"          },
    { IIC_INDEX_INA226_VCCINT_IO_BRAM, 200, "VccINT I/O BRAM" },
    { IIC_INDEX_INA226_VCC1V8,         200, "VCC 1V8"         },
    { IIC_INDEX_INA226_VCC1V2,         200, "VCC 1V2"         },
    { IIC_INDEX_INA226_VADJ_FMC,       200, "FMC Vadj"        },
    { IIC_INDEX_INA226_MGTAVCC,        500, "MGT AVcc"        },
    { IIC_INDEX_INA226_MGT1V2,         200, "MGT 1V2"         },
    { IIC_INDEX_INA226_MGT1V8C,        200, "MGT 1V8"         },
    { IIC_INDEX_INA226_VCCINT_RF,      200, "VCCINT RF"       },
    { IIC_INDEX_INA226_DAC_AVTT,       200, "DAC AVtt"        },
    { IIC_INDEX_INA226_DAC_AVCCAUX,    200, "DAC AVcc AUX"    },
    { IIC_INDEX_INA226_ADC_AVCC,       200, "ADC AVcc"        },
    { IIC_INDEX_INA226_ADC_AVCCAUX,    200, "ADC AVcc AUX"    },
    { IIC_INDEX_INA226_DAC_AVCC,       200, "DAC AVcc"        }
};

static struct {
    int     iicIndex;
    uint8_t page;
    uint8_t vReg;
    uint8_t iReg;
    const char *name;
} const pmbInfo[] = {
    { IIC_INDEX_IRPS5401_B,   0, 0x88, 0xFF, "Board 12V"      },
    { IIC_INDEX_IRPS5401_B,   0, 0x8B, 0x8C, "Utility 3.3V"   },
};

/*
 * Internal XSYSMON block
 */
static XSysMonPsu xSysmon;
static void
initBlock(int block)
{
    XSysMonPsu_SetAvg(&xSysmon, XSM_AVG_256_SAMPLES, block);
    XSysMonPsu_SetSeqAvgEnables(&xSysmon, XSYSMONPSU_SEQ_CH0_TEMP_MASK |
                                          XSYSMONPSU_SEQ_CH0_CALIBRTN_MASK,
                                          block);
    XSysMonPsu_SetSeqChEnables(&xSysmon, XSYSMONPSU_SEQ_CH0_TEMP_MASK |
                                         XSYSMONPSU_SEQ_CH0_CALIBRTN_MASK,
                                         block);
    XSysMonPsu_SetSequencerMode(&xSysmon, XSM_SEQ_MODE_CONTINPASS, block);
}
void
sysmonInit(void)
{
    XSysMonPsu_Config *configp;

    configp = XSysMonPsu_LookupConfig(XPAR_XSYSMONPSU_0_DEVICE_ID);
    if (configp == NULL) fatal("XSysMonPsu_LookupConfig");
    XSysMonPsu_CfgInitialize(&xSysmon, configp, configp->BaseAddress);
    initBlock(XSYSMON_PS);
    initBlock(XSYSMON_PL);
    sysmonDisplay();
}

/*
 * Read internal temperature and convert to units of 1/10 degree Celsius
 */
static int
readInternalTemperature(int block)
{
    int v = XSysMonPsu_GetAdcData(&xSysmon, XSM_CH_TEMP, block);
    return (((v * 501.3743) / 65536.0) - 273.6777) * 10.0;
}

static int
fetchVI(int ps, float *vp, float *ip)
{
    int i;
    uint8_t vBuf[2], iBuf[2];
    if (iicRead (psInfo[ps].iicIndex, 2, vBuf, 2)
     && iicRead (psInfo[ps].iicIndex, 1, iBuf, 2)) {
        i = (vBuf[0] << 8) + vBuf[1];
        *vp = i / 800.0;
        i = (iBuf[0] << 8) + iBuf[1];
        if (i & 0x8000) i -= 0x10000;
        i = i * 5 / 2; /* 2.5 uV per count */
        i *= psInfo[ps].ampsPerVolt;
        *ip = i / 1.0e6;
        return 1;
    }
    return 0;
}

static void
showTemperature(const char *name, int t)
{
    printf("%19s temperature:%3d.%d C\n", name, t / 10, t % 10);
}
void
sysmonDisplay(void)
{
    int ps;

    for (ps = 0 ; ps < sizeof psInfo / sizeof psInfo[0] ; ps++) {
        float v, i;
        if (fetchVI(ps, &v, &i)) {
            printf("%16s: %7.3f V  %8.3f A\n", psInfo[ps].name, v, i);
        }
    }
    showTemperature("PS SYSMON", readInternalTemperature(XSYSMON_PS));
    showTemperature("PL SYSMON", readInternalTemperature(XSYSMON_PL));
}

/*
 * Return system monitors
 */
int
sysmonFetch(uint32_t *args)
{
    int i;
    int aIndex = 0;
    int shift = 0;
    uint32_t v = 0;
    evrTimestamp now;

    evrCurrentTime(&now);
    args[aIndex++] = now.secPastEpoch;
    args[aIndex++] = now.ticks;

    for (i = 0 ; i < sizeof psInfo / sizeof psInfo[0] ; i++) {
        uint8_t vBuf[2] = {0}, iBuf[2] = {0};
#ifndef SYSMON_SKIP_PSINFO
        iicRead (psInfo[i].iicIndex, 2, vBuf, 2);
        iicRead (psInfo[i].iicIndex, 1, iBuf, 2);
#endif
        args[aIndex++] = (iBuf[0]<<24)|(iBuf[1]<<16)|(vBuf[0]<<8)|vBuf[1];
    }
    for (i = 0 ; i < sizeof pmbInfo / sizeof pmbInfo[0] ; i++) {
        int iicIndex = pmbInfo[i].iicIndex;
        int page = pmbInfo[i].page;
        args[aIndex] = 0;
#ifndef SYSMON_SKIP_PSINFO
        args[aIndex] = pmbusRead(iicIndex, page, pmbInfo[i].vReg) & 0xFFFF;
        if (pmbInfo[i].iReg != 0xFF) {
            args[aIndex] |= pmbusRead(iicIndex, page, pmbInfo[i].iReg) << 16;
        }
#endif
        aIndex++;
    }
    args[aIndex++] = (readInternalTemperature(XSYSMON_PS) << 16) |
                      readInternalTemperature(XSYSMON_PL);
    args[aIndex++] = frequencyMonitorGet(3); // ADC AXI
    args[aIndex++] = GPIO_READ(GPIO_IDX_EVR_SYNC_CSR);
    aIndex += sfpGetStatus(args + aIndex);
    for (i = IIC_INDEX_IR38064_A ; i <= IIC_INDEX_IRPS5401_C ; i++) {
        if (shift > 16) {
            args[aIndex++] = v;
            v = 0;
            shift = 0;
        }
#ifndef SYSMON_SKIP_PSINFO
        v |= ((pmbusRead(i, 0xFF, 0x8D)*10)/256) << shift;
#endif
        shift += 16;
    }
    args[aIndex++] = v;
    args[aIndex++] = GPIO_READ(GPIO_IDX_SYSREF_CSR);
    args[aIndex++] = rfADCstatus();
    args[aIndex++] = duplicateIOCcheck(0, 0);
    return aIndex;
}

/*
 * Show power supply name (right-adjusted)
 */
static void
showName(int x, int y, int w, const char *str)
{
    const char *cp = str + strlen(str);

    while ((x >= 0) && (cp-- != str)) {
        if (*cp == ' ') {
            x -= (w * 2) / 3;
        }
        else {
            st7789vDrawChar(x, y, *cp);
            x -= w;
        }
    }
}

void
sysmonDraw(int redrawAll, int page)
{
    char cbuf[16];
    int h = st7789vCharHeight;
    int w = st7789vCharWidth;
    static enum { drawingNames, drawingValues } state = drawingNames;
    static int currentPage = -1;
    static int charRowIndex, psIndex, base, count;
    static uint32_t thenLine, thenPage, usePageDelay;
    uint32_t now = MICROSECONDS_SINCE_BOOT();
    struct label { char *name; char *units; };
    static const struct label p2labels[] = {
                             { "PS Temperature",  "C" },
                             { "PL Temperature",  "C" },
                             { "SFP Temperature", "C" },
                             { "U68 Temperature", "C" },
                             { "U70 Temperature", "C" },
                             { "U74 Temperature", "C" },
                             { "U75 Temperature", "C" },
                             { "U53 Temperature", "C" },
                             { "U55 Temperature", "C" },
                             { "U57 Temperature", "C" },
                             { "EVR Rx Power",    "uW" }};

    if (redrawAll || (page != currentPage)) {
        state = drawingNames;
        switch (page) {
        case 0:
            base = 0;
            count = 1 + ((sizeof psInfo / sizeof psInfo[0]) / 2);
            break;
        case 1:
            base = (sizeof psInfo / sizeof psInfo[0]) / 2;
            count = 1 + ((sizeof psInfo / sizeof psInfo[0]) - base);
            break;
        default:
            base = 0;
            count = sizeof p2labels / sizeof p2labels[0];
        }
        psIndex = base;
        charRowIndex = 0;
        usePageDelay = 0;
    }
    else {
        if (usePageDelay) {
            if ((now - thenPage) < 1000000) return;
            thenPage = now;
            thenLine = now;
            usePageDelay = 0;
        }
        else {
            if ((now - thenLine) < 20000) return;
            thenLine = now;
        }
    }
    currentPage = page;

    /*
     * Draw next line of display
     */
    switch (state) {
    case drawingNames:
        if (page < 2) {
            const char *name;
            if (charRowIndex == 0) {
                st7789vShowString(DISPLAY_WIDTH-8*w, 0, "V    A");
            }
            if (charRowIndex == (count-1)) {
                name = page == 0 ? "Board 12V" : "Utility 3.3V";
            }
            else {
                name = psInfo[psIndex].name;
            }
            showName(DISPLAY_WIDTH - 12 * w, (charRowIndex + 1) * h, w, name);
        }
        else {
            if (charRowIndex < count) {
                const struct label *lp = &p2labels[charRowIndex];
                st7789vShowString(DISPLAY_WIDTH-3*w, charRowIndex*h, lp->units);
                showName(DISPLAY_WIDTH - 12 * w, charRowIndex * h, w, lp->name);
            }
        }
        break;
    case drawingValues:
        if (page < 2) {
            float v, i;
            if (charRowIndex == (count-1)) {
                if (page == 0) {
                    v = pmbusRead(IIC_INDEX_IRPS5401_B, 0, 0x88) / 256.0;
                    snprintf(cbuf, sizeof cbuf, "%4.1f", v);
                }
                else {
                    v = pmbusRead(IIC_INDEX_IRPS5401_B, 0, 0x8B) / 256.0;
                    i = pmbusRead(IIC_INDEX_IRPS5401_B, 0, 0x8C) / 256.0;
                    snprintf(cbuf, sizeof cbuf, "%4.2f%5.2f", v, i);
                }
            }
            else if (fetchVI(psIndex, &v, &i)) {
                snprintf(cbuf, sizeof cbuf, "%4.2f%5.2f", v, i);
            }
            st7789vShowString(DISPLAY_WIDTH-10*w, (charRowIndex+1)*h, cbuf);
        }
        else {
            int v;
            switch (charRowIndex) {
            case 0: v = readInternalTemperature(XSYSMON_PS);    break;
            case 1: v = readInternalTemperature(XSYSMON_PL);    break;
            case 2: v = sfpGetTemperature();                    break;
            default:
                v=(pmbusRead(IIC_INDEX_IR38064_A+charRowIndex-3, 0xFF, 0x8D)*10)/256;
                break;
            case (sizeof p2labels / sizeof p2labels[0]) - 1:
                v = sfpGetRxPower();
                break;
            }
            snprintf(cbuf, sizeof cbuf, "%3d.%d", v / 10, v % 10);
            st7789vShowString(DISPLAY_WIDTH - 9 * w, charRowIndex * h, cbuf);
        }
        break;
    }
    if (++charRowIndex >= count) {
        psIndex = base;
        charRowIndex = 0;
        if (state == drawingValues) {
            usePageDelay = 1;
        }
        state = drawingValues;
    }
    else {
        psIndex++;
    }
}
