/*
 * Global settings
 */

#ifndef _SYSTEM_PARAMETERS_H_
#define _SYSTEM_PARAMETERS_H_

#include <stdint.h>

struct sysNetParms {
    uint32_t  address;
    uint32_t  netmask;
    uint32_t  gateway;
};

struct sysNetConfig {
    unsigned char      ethernetMAC[8]; /* Pad to 4-byte boundary */
    struct sysNetParms ipv4;
};
extern struct sysNetConfig netDefault;

extern struct systemParameters {
    struct sysNetConfig netConfig;
    int                 userMGTrefClkOffsetPPM;
    int                 startupDebugFlags;
    int                 rfDivisor;
    int                 pllMultiplier;
    int                 isSinglePass;
    int                 evrPerFaMarker;
    uint32_t            checksum;
} systemParameters;


void systemParametersReadback(void);
void systemParametersStash(void);

char *formatIP(const void *val);
int   parseIP(const char *str, void *val);
char *formatMAC(const void *val);
int   parseMAC(const char *str, void *val);

void showNetworkConfiguration(const struct sysNetParms *ipv4);
void systemParametersShowUserMGTrefClkOffsetPPM(void);

#endif
