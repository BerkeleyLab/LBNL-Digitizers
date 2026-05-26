/*
 * Utility routines
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>
#include <lwip/ip4_addr.h>

/*
 * Convenient macros
 */
#define MB(x) ((x)*1024*1024)
#define kB(x) ((x)*1024)

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef member_size
#define member_size(TYPE, MEMBER)  (sizeof(((TYPE *)0)->MEMBER))
#endif

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

#define IP4_FORMAT(a,b,c,d) PP_HTONL(LWIP_MAKEU32(a,b,c,d))

/*
 * Diagnostics
 */
#define DEBUGFLAG_EPICS             0x01
#define DEBUGFLAG_TFTP              0x02
#define DEBUGFLAG_EVR               0x04
#define DEBUGFLAG_ACQUISITION       0x08
#define DEBUGFLAG_IIC               0x10
#define DEBUGFLAG_DRP               0x40
#define DEBUGFLAG_CALIBRATION       0x80
#define DEBUGFLAG_SHOW_TEST         0x100
#define DEBUGFLAG_SHOW_AFE_REG      0x200
#define DEBUGFLAG_SHOW_SYSREF       0x400
#define DEBUGFLAG_ADC_MMCM_SHOW     0x1000
#define DEBUGFLAG_RF_CLK_SHOW       0x2000
#define DEBUGFLAG_RF_ADC_SHOW       0x4000
#define DEBUGFLAG_SHOW_DRP          0x8000
#define DEBUGFLAG_SLIDE_MGT         0x20000
#define DEBUGFLAG_RESTART_AFE_ADC   0x100000
#define DEBUGFLAG_RESTART_TILES     0x200000
#define DEBUGFLAG_RESYNC_ADC        0x400000

extern int debugFlags;

void fatal(const char *fmt, ...);
void warn(const char *fmt, ...);
void microsecondSpin(unsigned int us);
void resetFPGA(void);
void checkForReset(void);
int resetRecoverySwitchPressed(void);
void showReg(unsigned int i);

int serialNumberDFE(void);
float mean(int32_t *numbers, int length);
void bufferDisplay(uint8_t *buf, int n);

#endif /* _UTIL_H_ */
