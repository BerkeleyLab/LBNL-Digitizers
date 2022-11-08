/*
 * Auto-trim support
 */

#ifndef _AUTOTRIM_H_
#define _AUTOTRIM_H_

#define AUTOTRIM_CSR_MODE_OFF               0x0
#define AUTOTRIM_CSR_MODE_LOW               0x1
#define AUTOTRIM_CSR_MODE_HIGH              0x2
#define AUTOTRIM_CSR_MODE_DUAL              0x3
#define AUTOTRIM_MODE_INBAND                0x4

#define AUTOTRIM_STATUS_DISABLED            0
#define AUTOTRIM_STATUS_NO_TONES            1
#define AUTOTRIM_STATUS_EXCESSIVE_VARIATION 2
#define AUTOTRIM_STATUS_SINGLE_SIDED        3
#define AUTOTRIM_STATUS_DOUBLE_SIDED        4

void         autotrimEnable(unsigned int bpm, int flag);
void         autotrimSetStaticGains(unsigned int bpm, unsigned int channel, int gain);
void         autotrimUsePulsePilot(unsigned int bpm, int flag);
int          autotrimStatus(unsigned int bpm);
void         autotrimSetThreshold(unsigned int bpm, unsigned int threshold);
unsigned int autotrimGetThreshold(unsigned int bpm);
void         autotrimSetFilterShift(unsigned int bpm, unsigned int filterShift);
unsigned int autotrimGetFilterShift(unsigned int bpm);

#endif
