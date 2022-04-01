/*
 * Application-specific EPICS support
 */
#ifndef _EPICS_APPLICATION_H_
#define _EPICS_APPLICATION_H_

int epicsApplicationCommand(int commandArgCount, struct hsdPacket *cmdp,
                                                 struct hsdPacket *replyp);

#endif  /* _EPICS_APPLICATION_H_ */
