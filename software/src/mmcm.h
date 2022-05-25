/*
 * MMCM reconfiguration
 */

#ifndef _MMCM_H_
#define _MMCM_H_

void mmcmInit(void);
void mmcmShow(void);
void mmcmSetAdcClkMultiplier(int multiplier);
void mmcmSetAdcClkDivider(int divider);

#endif /* _MMCM_H_ */
