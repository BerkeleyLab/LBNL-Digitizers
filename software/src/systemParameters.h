/*
 * Global settings
 */

#ifndef _SYSTEM_PARAMETERS_H_
#define _SYSTEM_PARAMETERS_H_

#include <stdint.h>
#include <lwip/ip_addr.h>

#define SYSTEM_PARAMETERS_NAME "sysParms.csv"

struct sysNetParms {
    uint32_t  address;
    uint32_t  netmask;
    uint32_t  gateway;
};

struct sysNetConfig {
    unsigned char      ethernetMAC[8]; /* Pad to 4-byte boundary */
    int                useDHCP;
    struct sysNetParms ipv4;
};
extern struct sysNetConfig netDefault;
extern struct sysNetConfig currentNetConfig;

extern struct systemParameters {
    struct sysNetConfig netConfig;
    int                 userMGTrefClkOffsetPPM;
    int                 startupDebugFlags;
    uint32_t            checksum;
} systemParameters;


void systemParametersSetDefaults(void);
int systemParametersFetchEEPROM(void);
int systemParametersStashEEPROM(void);
void systemParametersCommit(void);

void setDefaultNetAddress(struct sysNetConfig *netConfig,
        struct sysNetConfig *sysParamsNetConfig,
        struct sysNetConfig *defaultNetConfig, int isRecovery,
        uint8_t *eepromMAC, int size);
void showNetworkConfiguration(const struct sysNetParms *ipv4);
void systemParametersShowUserMGTrefClkOffsetPPM(void);

#endif
