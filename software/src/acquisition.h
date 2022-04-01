/*
 * Data acquisition
 */
#ifndef _ACQUISITION_H_
#define _ACQUISITION_H_

void acquisitionInit(void);
void acquisitionCrank(void);
void acquisitionArm(int channel, int enable);
uint32_t acquisitionStatus(void);
int acquisitionFetch(uint32_t*buf,int capacity,int channel,int offset,int last);
void acquisitionScaleChanged(int channel);

void acquisitionSetTriggerEdge(int channel, int v);
void acquisitionSetTriggerLevel(int channel, int microvolts);
void acquisitionSetTriggerEnables(int channel, int mask);
void acquisitionSetBonding(int channel, int bonded);
void acquisitionSetPretriggerCount(int channel, int n);
void acquisitionSetSegmentedMode(int channel, int isSegmented);
void acquisitionSetEarlySegmentInterval(int channel, int adcClockTicks);
void acquisitionSetLaterSegmentInterval(int channel, int adcClockTicks);

#endif  /* _ACQUISITION_H_ */
