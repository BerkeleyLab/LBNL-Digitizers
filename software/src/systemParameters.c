#include <stdio.h>
#include <xil_io.h>
#include <lwip/def.h>
#include "gpio.h"
#include "iic.h"
#include "systemParameters.h"
#include "user_mgt_refclk.h"
#include "util.h"

struct systemParameters systemParameters;
struct sysNetConfig netDefault;

static int
checksum(void)
{
    int i, sum = 0xCAFEF00D;
    const int *ip = (int *)&systemParameters;

    for (i = 0 ; i < ((sizeof systemParameters -
                    sizeof systemParameters.checksum) / sizeof(*ip)) ; i++)
        sum += *ip++ + i;
    if (sum == 0) sum = 0xABCD0341;
    return sum;
}

void systemParametersUpdateChecksum(void)
{
    systemParameters.checksum = checksum();
}

/*
 * Read and process values on system startup.
 * Perform sanity check on parameters read from EEPROM.
 * If they aren't good then assign default values.
 */
void
systemParametersReadback(void)
{
    netDefault.ethernetMAC[0] = 0xAA;
    netDefault.ethernetMAC[1] = 'L';
    netDefault.ethernetMAC[2] = 'B';
    netDefault.ethernetMAC[3] = 'N';
    netDefault.ethernetMAC[4] = 'L';
    netDefault.ethernetMAC[5] = 0x01;
    netDefault.ipv4.address = htonl((192<<24) | (168<<16) | (  1<< 8) | 128);
    netDefault.ipv4.netmask = htonl((255<<24) | (255<<16) | (255<< 8) | 0);
    netDefault.ipv4.gateway = htonl((192<<24) | (168<<16) | (  1<< 8) | 1);
    if ((eepromRead(0, &systemParameters, sizeof systemParameters) < 0)
     || (checksum() != systemParameters.checksum)) {
        printf("\n====== ASSIGNING DEFAULT PARAMETERS ===\n\n");
        systemParameters.netConfig = netDefault;
        systemParameters.userMGTrefClkOffsetPPM = 0;
        systemParameters.startupDebugFlags = 0;
        systemParameters.rfDivisor = 357;
        systemParameters.pllMultiplier = 80;
        systemParameters.isSinglePass = 0;
        systemParameters.adcHeartbeatMarker = 480 * 119 * 2000;
        systemParameters.evrPerFaMarker = 480 * 119;
        systemParameters.evrPerSaMarker = 480 * 119 * 200;
    }
    debugFlags = systemParameters.startupDebugFlags;
    if (userMGTrefClkAdjust(systemParameters.userMGTrefClkOffsetPPM)) {
        systemParametersShowUserMGTrefClkOffsetPPM();
    }
}

/*
 * Update EEPROM
 */
void
systemParametersStash(void)
{
    systemParametersUpdateChecksum();
    if (eepromWrite(0, &systemParameters, sizeof systemParameters) < 0) {
        printf("Unable to write system parameteres to EEPROM.\n");
    }
}

/*
 * Serializer/deserializers
 * Note -- Format routines share common static buffer.
 */
static char cbuf[40];
char *
formatMAC(const void *val)
{
    const unsigned char *addr = (const unsigned char *)val;
    sprintf(cbuf, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1], addr[2],
                                                   addr[3], addr[4], addr[5]);
    return cbuf;
}

int
parseMAC(const char *str, void *val)
{
    const char *cp = str;
    int i = 0;
    long l;
    char *endp;

    for (;;) {
        l = strtol(cp, &endp, 16);
        if ((l < 0) || (l > 255))
            return -1;
        *((uint8_t*)val + i) = l;
        if (++i == 6)
            return endp - str;
        if (*endp++ != ':')
            return -1;
        cp = endp;
    }
}

char *
formatIP(const void *val)
{
    uint32_t l = ntohl(*(uint32_t *)val);
    sprintf(cbuf, "%d.%d.%d.%d", (int)(l >> 24) & 0xFF, (int)(l >> 16) & 0xFF,
                                 (int)(l >>  8) & 0xFF, (int)(l >>  0) & 0xFF);
    return cbuf;
}

int
parseIP(const char *str, void *val)
{
    const char *cp = str;
    uint32_t addr = 0;
    int i = 0;
    long l;
    char *endp;

    for (;;) {
        l = strtol(cp, &endp, 10);
        if ((l < 0) || (l > 255))
            return -1;
        addr = (addr << 8) | l;
        if (++i == 4) {
            *(uint32_t *)val = htonl(addr);
            return endp - str;
        }
        if (*endp++ != '.')
            return -1;
        cp = endp;
    }
}

void
showNetworkConfiguration(const struct sysNetParms *ipv4)
{
    printf("   IP ADDR: %s\n", formatIP(&ipv4->address));
    printf("  NET MASK: %s\n", formatIP(&ipv4->netmask));
    printf("   GATEWAY: %s\n", formatIP(&ipv4->gateway));
}

void
systemParametersShowUserMGTrefClkOffsetPPM(void)
{
    printf("User MGT reference clock offset: %d PPM\n",
                                       systemParameters.userMGTrefClkOffsetPPM);
}
