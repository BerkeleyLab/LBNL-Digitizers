/*
 * Manipulate local oscillator tables
 */

#ifndef _LOCAL_OSC_H_
#define _LOCAL_OSC_H_

int localOscGetRfTable(unsigned char *buf, int capacity);
int localOscSetRfTable(unsigned char *buf, int size);
void localOscRfReadback(const unsigned char *buf);
int localOscGetPtTable(unsigned char *buf, int capacity);
int localOscSetPtTable(unsigned char *buf, int size);
void localOscPtReadback(const unsigned char *buf);
void localOscRfCommit(void);
void localOscPtCommit(void);

void localOscillatorInit(void);

#endif
