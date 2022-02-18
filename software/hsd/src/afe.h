/*
 * Communicate with analog front end SPI components
 */

#ifndef _AFE_SPI_H_
#define _AFE_SPI_H_

#define AFE_COUPLING_DC             0
#define AFE_COUPLING_AC             1
#define AFE_COUPLING_CALIBRATION    2
#define AFE_COUPLING_TRAINING       3
#define AFE_COUPLING_OPEN           4

#define AFE_EEPROM_NAME "AFEEPROM.bin"

void afeInit(void);
void afeStart(void);
void afeADCrestart(void);
int afeGetSerialNumber(void);
int afeSetCoupling(unsigned int channel, int coupling);
int afeSetGain(unsigned int channel, int gainCode);
int afeSetDAC(int value);
void afeEnableTrainingTone(int enable);
int lmh6401GetRegister(unsigned int channel, int r);
void afeShow(void);
int afeFetchCalibration(unsigned int channel, uint32_t *buf);
void afeFetchEEPROM(void);
void afeStashEEPROM(void);
int afeMicrovoltsToCounts(unsigned int channel, int microVolts);
int afeFetchADCextents(uint32_t *buf);

#endif /* _AFE_SPI_H_ */
