#ifndef _HIGH_SPEED_DIGITIZER_PROTOCOL_
#define _HIGH_SPEED_DIGITIZER_PROTOCOL_

#if defined(PLATFORM_ZYNQMP) || defined(PLATFORM_VERSAL)
# include <stdint.h>
  typedef uint8_t  epicsUInt8;
  typedef int8_t   epicsInt8;
  typedef uint16_t epicsUInt16;
  typedef int16_t  epicsInt16;
  typedef uint32_t epicsUInt32;
  typedef int32_t  epicsInt32;
#else
# include <epicsTypes.h>
#endif

#define HSD_PROTOCOL_UDP_PORT        50005
#define HSD_PROTOCOL_PUBLISHER_UDP_PORT 50006
#define HSD_PROTOCOL_MAGIC           0xBD008426
#define HSD_PROTOCOL_MAGIC_SWAPPED   0x268400BD
#define HSD_PROTOCOL_MAGIC_SLOW_ACQUISITION  0xCAFE0004
#define HSD_PROTOCOL_ARG_CAPACITY    350
#define HSD_PROTOCOL_ADC_COUNT       8
#define HSD_PROTOCOL_DSP_COUNT       2

struct hsdPacket {
    epicsUInt32    magic;
    epicsUInt32    nonce;
    epicsUInt32    command;
    epicsUInt32    args[HSD_PROTOCOL_ARG_CAPACITY];
};

/*
 * Slow acquisition (typically 10 Hz) monitoring
 */
struct hsdSlowAcquisition {
    epicsUInt32 magic;
    epicsUInt32 packetNumber;
    epicsUInt32 seconds;
    epicsUInt32 ticks;
    epicsUInt8  syncStatus;
    epicsUInt8  recorderStatus;
    epicsUInt8  clipStatus;
    epicsUInt8  cellCommStatus;
    epicsUInt8  autotrimStatus;
    epicsUInt8  sdSyncStatus;
    epicsUInt8  pad1;
    epicsUInt8  pad2;
    epicsUInt16 adcPeak[HSD_PROTOCOL_ADC_COUNT];
    epicsUInt32 rfMag[HSD_PROTOCOL_ADC_COUNT];
    epicsUInt32 ptLoMag[HSD_PROTOCOL_ADC_COUNT];
    epicsUInt32 ptHiMag[HSD_PROTOCOL_ADC_COUNT];
    epicsUInt32 gainFactor[HSD_PROTOCOL_ADC_COUNT];
    epicsInt32  xPos[HSD_PROTOCOL_DSP_COUNT];
    epicsInt32  yPos[HSD_PROTOCOL_DSP_COUNT];
    epicsInt32  skew[HSD_PROTOCOL_DSP_COUNT];
    epicsInt32  buttonSum[HSD_PROTOCOL_DSP_COUNT];
    epicsInt32  xRMSwide[HSD_PROTOCOL_DSP_COUNT];
    epicsInt32  yRMSwide[HSD_PROTOCOL_DSP_COUNT];
    epicsInt32  xRMSnarrow[HSD_PROTOCOL_DSP_COUNT];
    epicsInt32  yRMSnarrow[HSD_PROTOCOL_DSP_COUNT];
};

#define HSD_PROTOCOL_SIZE_TO_ARG_COUNT(s) (HSD_PROTOCOL_ARG_CAPACITY - \
                    ((sizeof(struct hsdPacket)-(s))/sizeof(epicsUInt32)))
#define HSD_PROTOCOL_ARG_COUNT_TO_SIZE(a) (sizeof(struct hsdPacket) - \
                        ((HSD_PROTOCOL_ARG_CAPACITY - (a)) * sizeof(epicsUInt32)))
#define HSD_PROTOCOL_ARG_COUNT_TO_U32_COUNT(a) \
                    ((sizeof(struct hsdPacket) / sizeof(epicsUInt32)) - \
                                            (HSD_PROTOCOL_ARG_CAPACITY - (a)))
#define HSD_PROTOCOL_U32_COUNT_TO_ARG_COUNT(u) (HSD_PROTOCOL_ARG_CAPACITY - \
                    (((sizeof(struct hsdPacket)/sizeof(epicsUInt32)))-(u)))

#define HSD_PROTOCOL_CMD_MASK_HI             0xF000
#define HSD_PROTOCOL_CMD_MASK_LO             0x0F80
#define HSD_PROTOCOL_CMD_MASK_IDX            0x007F

#define HSD_PROTOCOL_CMD_HI_LONGIN           0x0000
# define HSD_PROTOCOL_CMD_LONGIN_IDX_FIRMWARE_BUILD_DATE 0x00
# define HSD_PROTOCOL_CMD_LONGIN_IDX_SOFTWARE_BUILD_DATE 0x01
# define HSD_PROTOCOL_CMD_LONGIN_IDX_LMX2594_STATUS      0x02
# define HSD_PROTOCOL_CMD_LONGIN_IDX_ACQ_STATUS          0x03
# define HSD_PROTOCOL_CMD_LONGIN_IDX_DFE_SERIAL_NUMBER   0x04
# define HSD_PROTOCOL_CMD_LONGIN_IDX_AFE_SERIAL_NUMBER   0x05

#define HSD_PROTOCOL_CMD_HI_LONGOUT          0x1000
# define HSD_PROTOCOL_CMD_LONGOUT_LO_NO_VALUE        0x0000
#  define HSD_PROTOCOL_CMD_LONGOUT_NV_IDX_CLEAR_POWERUP_STATUS  0x00
#  define HSD_PROTOCOL_CMD_LONGOUT_NV_IDX_SOFT_TRIGGER          0x01
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_SEGMENTED_MODE     0x0100
# define HSD_PROTOCOL_CMD_LONGOUT_LO_EARLY_SEGMENT_INTERVAL 0x0200
# define HSD_PROTOCOL_CMD_LONGOUT_LO_LATER_SEGMENT_INTERVAL 0x0300
# define HSD_PROTOCOL_CMD_LONGOUT_LO_TRIGGER_LEVEL          0x0400
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_TRIGGER_ENABLES    0x0500
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_PRETRIGGER_SAMPLES 0x0600
# define HSD_PROTOCOL_CMD_LONGOUT_LO_REARM                  0x0700
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_GAIN               0x0800
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_EVENT_ACTION       0x0900
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_TRIGGER_DELAY      0x0A00
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_COUPLING           0x0C00
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_TRIGGER_EDGE       0x0D00
# define HSD_PROTOCOL_CMD_LONGOUT_LO_SET_INPUT_BONDING      0x0E00
# define HSD_PROTOCOL_CMD_LONGOUT_LO_GENERIC                0x0F00
#  define HSD_PROTOCOL_CMD_LONGOUT_GENERIC_IDX_REBOOT           0x00
#  define HSD_PROTOCOL_CMD_LONGOUT_GENERIC_EVR_CLK_PER_TURN     0x01
#  define HSD_PROTOCOL_CMD_LONGOUT_GENERIC_ENABLE_TRAINING_TONE 0x02
#  define HSD_PROTOCOL_CMD_LONGOUT_GENERIC_SET_CALIBRATION_DAC  0x03

#define HSD_PROTOCOL_CMD_HI_SYSMON           0x2000
# define HSD_PROTOCOL_CMD_SYSMON_LO_INT32       0x0000
# define HSD_PROTOCOL_CMD_SYSMON_LO_UINT16_LO   0x0100
# define HSD_PROTOCOL_CMD_SYSMON_LO_UINT16_HI   0x0200
# define HSD_PROTOCOL_CMD_SYSMON_LO_INT16_LO    0x0300
# define HSD_PROTOCOL_CMD_SYSMON_LO_INT16_HI    0x0400

#define HSD_PROTOCOL_CMD_HI_WAVEFORM         0x3000

#define HSD_PROTOCOL_CMD_HI_PLL_CONFIG       0x4000
# define HSD_PROTOCOL_CMD_PLL_CONFIG_LO_SET     0x0000
# define HSD_PROTOCOL_CMD_PLL_CONFIG_LO_GET     0x0100

#endif /* _HIGH_SPEED_DIGITIZER_PROTOCOL_ */
