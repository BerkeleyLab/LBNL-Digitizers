/*
 * Data acquisition
 */
#ifndef _ACQUISITION_H_
#define _ACQUISITION_H_

void acquisitionInit(void);
void acquisitionCrank(void);
void acquisitionArm(int channel, int enable);
int acquisitionStatus(uint32_t status[], int capacity);
int acquisitionFetch(uint32_t*buf,int capacity,int channel,int offset,int last);
void acquisitionScaleChanged(int channel);

void acquisitionSetTriggerEdge(int channel, int v);
void acquisitionSetTriggerLevel(int channel, int microvolts);
void acquisitionSetTriggerEnables(int channel, int mask);
void acquisitionSetBonding(int channel, int bonded);
void acquisitionSetPretriggerCount(int channel, int n);
void acquisitionSetSegmentedMode(int channel, int segMode);
void acquisitionSetSegmentedMeanMode(int channel, int segMeanMode);
void acquisitionSetEarlySegmentInterval(int channel, int adcClockTicks);
void acquisitionSetLaterSegmentInterval(int channel, int adcClockTicks);

// Only for BCM styule
void acquisitionSetSize(unsigned int n);
void acquisitionSetPassCount(unsigned int n);

#endif  /* _ACQUISITION_H_ */
