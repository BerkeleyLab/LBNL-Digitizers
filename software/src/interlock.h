/*
 * Interlock I/O
 */
#ifndef _INTERLOCK_H_
#define _INTERLOCK_H_

#define INTERLOCK_STATE_RELAY_OPEN          0
#define INTERLOCK_STATE_RELAY_CLOSED        1
#define INTERLOCK_STATE_FAULT_NOT_OPEN      2
#define INTERLOCK_STATE_FAULT_NOT_CLOSED    3
#define INTERLOCK_STATE_FAULT_BOTH_OFF      4
#define INTERLOCK_STATE_FAULT_BOTH_ON       5

int interlockRelayState(void);
int interlockButtonPressed(void);

#endif  /* _INTERLOCK_H_ */
