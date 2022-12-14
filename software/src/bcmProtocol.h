/*
 * Additions for bunch current monitor
 */

#ifndef _BUNCH_CURRENT_MONITOR_PROTOCOL_
#define _BUNCH_CURRENT_MONITOR_PROTOCOL_

#define BCM_PROTOCOL_CMD_LONGOUT_ACQUISITION_SIZE       0x10
#define BCM_PROTOCOL_CMD_LONGOUT_PASS_COUNT             0x11
#define BCM_PROTOCOL_CMD_LONGOUT_EVENT_TRIGGER          0x12
#define BCM_PROTOCOL_CMD_LONGOUT_EVENT_INJECTION        0x13

/*
 * Acquisition status bits
 */
#define BCM_PROTOCOL_ACQ_FOLLOWS_INJECTION  0x80000000
#define BCM_PROTOCOL_ACQ_SOFTWARE_TRIGGER   0x40000000

#endif /* _BUNCH_CURRENT_MONITOR_PROTOCOL_ */

