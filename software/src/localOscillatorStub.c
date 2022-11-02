/*
 * Stub for local oscillator tables API
 */

#include "localOscillator.h"

int
localOscSetRfTable(unsigned char *buf, int size)
{
    return 0;
}

int
localOscSetPtTable(unsigned char *buf, int size)
{
    return 0;
}

int
localOscGetRfTable(unsigned char *buf, int size)
{
    return 0;
}

int
localOscGetPtTable(unsigned char *buf, int size)
{
    return 0;
}

void
sdAccumulateSetTbtSumShift(int shift)
{
}

void
sdAccumulateSetMtSumShift(int shift)
{
}

void
sdAccumulateSetFaSumShift(int shift)
{
}

void
localOscRfReadback(const unsigned char *buf)
{
}

void
localOscPtReadback(const unsigned char *buf)
{
}

void
localOscRfCommit(void)
{
}

void
localOscPtCommit(void)
{
}

void
localOscSetDspAlgorithm(int useRMS)
{
}

int
localOscGetDspAlgorithm(void)
{
    return 1;
}

int
localOscGetSdSyncStatus(void)
{
    return 0;
}

void localOscillatorInit(void)
{
}
