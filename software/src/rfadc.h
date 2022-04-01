/*
 * RF Analog to Digital Data Conversion
 */
#ifndef _RFADC_H_
#define _RFADC_H_

void rfADCinit(void);
void rfADCsync(void);
void rfADCrestart(void);
void rfADCfreezeCalibration(int channel, int freeze);
void rfADCshow(void);
unsigned int rfADCstatus(void);
int rfADClinkCouplingIsAC(void);

#endif  /* _RFADC_H_ */
