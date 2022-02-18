/*
 * Perform transceiver eye scans
 *
 * This code is based on the example provided in Xilinx application note
 * XAPP743, "Eye Scan with MicroBlaze Processor MCS Application Note".
 */

#ifndef _EYESCAN_H_
#define _EYESCAN_H_

/*
 * Set this to the comma-separated list of GTY names for your application.
 * Set the first character of a name to '>' for lanes greater than 10 Gb/s.
 */
#define EYESCAN_LANE_NAMES { "EVR" }

void eyescanInit(void);
int eyescanCrank(void);
int eyescanCommand (int argc, char **argv);

#endif /* _EYESCAN_H_ */
