/*
 * EPICS IOC communication
 */
#ifndef _EPICS_H_
#define _EPICS_H_

void epicsInit(void);
int duplicateIOCcheck(unsigned long address, unsigned int port);

#endif  /* _EPICS_H_ */
