/*
 * System monitoring
 */
#ifndef _SYSMON_H_
#define _SYSMON_H_

void sysmonInit(void);
void sysmonDisplay(void);
int sysmonFetch(uint32_t *args);
void sysmonDraw(int redrawAll, int page);

#endif  /* _SYSMON_H_ */
