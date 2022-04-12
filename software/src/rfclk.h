/*
 * RF clockk generation components
 */
#ifndef _RFCLK_H_
#define _RFCLK_H_

void rfClkInit(void);
void rfClkInit04208(void);
void rfClkShow(void);
void lmx2594Config(int muxSelect, const uint32_t *values, int n);
void lmx2594ConfigAllSame(const uint32_t *values, int n);
int lmx2594Readback(int muxSelect, uint32_t *values, int capacity);
int lmx2594ReadbackFirst(uint32_t *values, int capacity);
int lmx2594Status(void);

#endif  /* _RFCLK_H_ */
