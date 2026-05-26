#include <stdio.h>
#include <xil_io.h>
#include <lwip/def.h>
#include <stdbool.h>
#include "gpio.h"
#include "iic.h"
#include "systemParameters.h"
#include "user_mgt_refclk.h"
#include "util.h"
#include "ffs.h"
#include "serdes.h"
#include "rfadc.h"

struct systemParameters systemParameters;
struct systemParameters systemParametersCandidate;

/*
 * Allow space for extra terminating '\0'
 */
#define MAX_SYSTEM_PARAMETERS_BUF_SIZE (1+MB(1))

struct sysNetConfig netDefault;
struct sysNetConfig currentNetConfig;
struct systemParameters systemParametersDefault;

/*
 * Buffer for file I/O
 */
static unsigned char systemParametersBuf[MAX_SYSTEM_PARAMETERS_BUF_SIZE];
static int systemParametersGetTable(unsigned char *buf, int capacity,
        const struct systemParameters* sysParams);
static int systemParametersSetTable(struct systemParameters* sysParams,
        const unsigned char *buf, int size);

static int
checksum(struct systemParameters *sysParams)
{
    int i, sum = 0xCAFEF00D;
    const int *ip = (int *)sysParams;

    for (i = 0 ; i < ((sizeof *sysParams -
                    sizeof sysParams->checksum) / sizeof(*ip)) ; i++)
        sum += *ip++ + i;
    if (sum == 0) sum = 0xABCD0341;
    return sum;
}

static
void systemParametersUpdateChecksum(struct systemParameters *sysParams)
{
    sysParams->checksum = checksum(sysParams);
}

void
systemParametersSetDefaults(void)
{
    netDefault.ethernetMAC[0] = 0xAA;
    netDefault.ethernetMAC[1] = 'L';
    netDefault.ethernetMAC[2] = 'B';
    netDefault.ethernetMAC[3] = 'N';
    netDefault.ethernetMAC[4] = 'L';
    netDefault.ethernetMAC[5] = 0x01;
    netDefault.ipv4.address = IP4_FORMAT(192, 168, 1, 128);
    netDefault.ipv4.netmask = IP4_FORMAT(255, 255, 255, 0);
    netDefault.ipv4.gateway = IP4_FORMAT(192, 168, 1, 1);
    netDefault.useDHCP = 0;
    systemParametersDefault.netConfig = netDefault;
    systemParametersDefault.userMGTrefClkOffsetPPM = 0;
    systemParametersDefault.startupDebugFlags = 0;
}

/*
 * Read and process values on system startup.
 * Perform sanity check on parameters read from EEPROM.
 * If they aren't good then assign default values.
 */
void
systemParametersCommit(void)
{
    if (checksum(&systemParametersCandidate) != systemParametersCandidate.checksum) {
        printf("\n====== ASSIGNING DEFAULT PARAMETERS ===\n\n");
        systemParametersCandidate = systemParametersDefault;
    }

    systemParameters = systemParametersCandidate;
    debugFlags = systemParameters.startupDebugFlags;

    if (userMGTrefClkAdjust(systemParameters.userMGTrefClkOffsetPPM)) {
        systemParametersShowUserMGTrefClkOffsetPPM();
    }
}

/*
 * Conversion table
 */
static struct conv {
    const char *name;
    size_t      offset;
    size_t      size;
    bool        visited;
    char     *(*format)(const void *val, size_t num);
    int       (*parse)(const char *str, void *val);
} conv[] = {
    {
        .name = "Ethernet Address",
        .offset = offsetof(struct systemParameters, netConfig.ethernetMAC),
        .size = member_size(struct systemParameters, netConfig.ethernetMAC),
        .visited = false,
        .format = formatMAC,
        .parse= parseMAC,
    },
    {
        .name = "Use DHCP",
        .offset = offsetof(struct systemParameters, netConfig.useDHCP),
        .size = member_size(struct systemParameters, netConfig.useDHCP),
        .visited = false,
        .format = formatInt,
        .parse = parseInt,
    },
    {
        .name = "IP Address",
        .offset = offsetof(struct systemParameters, netConfig.ipv4.address),
        .size = member_size(struct systemParameters, netConfig.ipv4.address),
        .visited = false,
        .format = formatIP,
        .parse = parseIP,
    },
    {
        .name = "IP Netmask",
        .offset = offsetof(struct systemParameters, netConfig.ipv4.netmask),
        .size = member_size(struct systemParameters, netConfig.ipv4.netmask),
        .visited = false,
        .format = formatIP,
        .parse = parseIP,
    },
    {
        .name = "IP Gateway",
        .offset = offsetof(struct systemParameters, netConfig.ipv4.gateway),
        .size = member_size(struct systemParameters, netConfig.ipv4.gateway),
        .visited = false,
        .format = formatIP,
        .parse = parseIP,
    },
    {
        .name = "User MGT ref offset",
        .offset = offsetof(struct systemParameters, userMGTrefClkOffsetPPM),
        .size = member_size(struct systemParameters, userMGTrefClkOffsetPPM),
        .visited = false,
        .format = formatInt,
        .parse = parseInt,
    },
    {
        .name = "Startup debug flags",
        .offset = offsetof(struct systemParameters, startupDebugFlags),
        .size = member_size(struct systemParameters, startupDebugFlags),
        .visited = false,
        .format = formatHex,
        .parse = parseHex,
    },
};

/*
 * String matcher
 *   Ignore case.
 */
static int
parameterMatch(const char *name, const char *match, size_t size)
{
    if (strncasecmp(name, match, size) == 0) {
        return 1;
    }

    return 0;
}

static int
searchConvName(struct conv *cnv, size_t size,
        const char *match, size_t len)
{
    int i = 0;
    int matched = -1;

    for (i = 0; i < size; ++i) {
        if (parameterMatch(cnv[i].name, match, len) ){
            if (matched >= 0) {
                // Duplicated match
                return -1;
            }

            matched = i;
        }
    }

    // Not found
    if (matched < 0) {
        return size;
    }

    return matched;
}

static int
searchConvFirstNonVisited(struct conv *cnv, size_t size)
{
    int i = 0;

    for (i = 0; i < size; ++i) {
        if (!cnv[i].visited){
            return i;
        }
    }

    // Not found
    return size;
}

/*
 * Serialize/deserialize complete table
 */
int
systemParametersGetTable(unsigned char *buf, int capacity,
        const struct systemParameters* sysParams)
{
    char *cp = (char *)buf;
    int i;

    for (i = 0 ; i < (sizeof conv / sizeof conv[0]) ; i++)
        cp += sprintf(cp, "%s,%s\n", conv[i].name, (*conv[i].format)(
                    (char *) sysParams + conv[i].offset, conv[i].size));
    return cp - (char *)buf;
}

/*
 * EEPROM I/O
 */
int
systemParametersFetchEEPROM(void)
{
    const char *name = SYSTEM_PARAMETERS_NAME;
    int nRead;
    FRESULT fr;
    FIL fil;
    UINT nWritten;

    fr = f_open(&fil, name, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) {
        return -1;
    }

    nRead = systemParametersGetTable((unsigned char *)systemParametersBuf,
            sizeof(systemParametersBuf), &systemParameters);
    if (nRead <= 0) {
        f_close(&fil);
        return -1;
    }

    fr = f_write(&fil, systemParametersBuf, nRead, &nWritten);
    if (fr != FR_OK) {
        f_close(&fil);
        return -1;
    }

    f_close(&fil);
    return nWritten;
}

static int
parseTable(struct systemParameters *sysParams,
        const unsigned char *buf, int size, char **err)
{
    const char *base = (const char *)buf;
    const char *cp = base;
    int pos = 0;
    int l = 0;
    int line = 1;

    /*
     * Step 1: Parse line, which must be "<parameter_name>", <parameter_value>\n
     * Step 2: Convert <parameter_value> according to the function in "conv"
     */
    while ((cp - base) < size) {
        size_t spn = strcspn (cp, ",");
        if (((cp - base) + spn) >= size) {
            *err = "Unexpected EOF";
            return -line;
        }

        pos = searchConvName(conv, ARRAY_SIZE(conv), cp, spn);
        if (pos < 0) {
            *err = "Duplicated parameter name";
            return -line;
        }

        // Ignore unexpected values
        if (pos >= ARRAY_SIZE(conv)) {
            printf ("Skiping unexpected parameter \"%.*s\"\n",
                    (int) spn, cp);
            spn = strcspn (cp, "\n");
            cp += spn + 1;
            line++;
            continue;
        }

        // Skip ','
        cp += spn + 1;

        l = (*conv[pos].parse)(cp, (char *)sysParams + conv[pos].offset);
        if (l <= 0) {
            *err = "Invalid value";
            return -line;
        }

        cp += l;

        // Mark node as visited
        conv[pos].visited = true;

        // Skip trailing spaces and ',' until a '\n' is found
        while (((cp - base) < size)
            && (((isspace((unsigned char)*cp))) || (*cp == ','))) {
            if (*cp == '\n') {
                line++;
            }
            cp++;
        }
    }

    // Check if every node was visited
    pos = searchConvFirstNonVisited(conv, ARRAY_SIZE(conv));
    if (pos < ARRAY_SIZE(conv)) {
        *err = "Missing parameters";
        return -line;
    }

    return cp - base;
}

int
systemParametersSetTable(struct systemParameters *sysParams,
        const unsigned char *buf, int size)
{
    char *err = "";
    int i = parseTable(sysParams, buf, size, &err);

    if (i <= 0) {
        printf("Bad file contents at line %d: %s\n", -i, err);
        return -1;
    }

    systemParametersUpdateChecksum(sysParams);
    return sizeof (*sysParams);
}

int
systemParametersStashEEPROM(void)
{
    const char *name = SYSTEM_PARAMETERS_NAME;
    int nWrite;
    FRESULT fr;
    FIL fil;
    UINT nRead;

    fr = f_open(&fil, name, FA_READ);
    if (fr != FR_OK) {
        return -1;
    }

    fr = f_read(&fil, systemParametersBuf, sizeof(systemParametersBuf)-1, &nRead);
    if (fr != FR_OK) {
        f_close(&fil);
        return -1;
    }
    if (nRead == 0) {
        f_close(&fil);
        return 0;
    }

    systemParametersBuf[nRead] = '\0';

    nWrite = systemParametersSetTable(&systemParametersCandidate,
            (unsigned char *)systemParametersBuf, nRead);
    f_close(&fil);

    return nWrite;
}

static const int MAC_ADDR_SIZE = 6;

static int
validateMACAddress(uint8_t *buf, int size)
{
    int ones = 0;
    int zeros = 0;

    if (size < MAC_ADDR_SIZE) {
        return -1;
    }

    for (int i = 0; i < MAC_ADDR_SIZE; i++) {
        if (buf[i] == 0) {
            zeros++;
        }

        if (buf[i] == 255) {
            ones++;
        }
    }

    if (zeros == 6 || ones == 6) {
        return -1;
    }

    return 0;
}


void
setDefaultNetAddress(struct sysNetConfig *netConfig,
        struct sysNetConfig *sysParamsNetConfig,
        struct sysNetConfig *defaultNetConfig, int isRecovery,
        uint8_t *eepromMAC, int size)
{
    if (isRecovery) {
        printf("\n====== RECOVERY MODE. ASSIGNING DEFAULT NETWORK PARAMETERS ===\n\n");
        netConfig->ipv4 = defaultNetConfig->ipv4;
        netConfig->useDHCP = defaultNetConfig->useDHCP;
        memcpy(netConfig->ethernetMAC, defaultNetConfig->ethernetMAC,
                sizeof (netConfig->ethernetMAC));
    }
    else {
        netConfig->useDHCP = sysParamsNetConfig->useDHCP;
#if LWIP_DHCP==1
        if (netConfig->useDHCP) {
            netConfig->ipv4.address = 0;
            netConfig->ipv4.netmask = 0;
            netConfig->ipv4.gateway = 0;
        }
        else {
            netConfig->ipv4 = sysParamsNetConfig->ipv4;
        }
#else
        netConfig->ipv4 = sysParamsNetConfig->ipv4;
#endif

        if (!eepromMAC) {
            memcpy(netConfig->ethernetMAC, sysParamsNetConfig->ethernetMAC,
                    sizeof (netConfig->ethernetMAC));
            printf("System Parameters MAC:\n");
            bufferDisplay(netConfig->ethernetMAC, sizeof (netConfig->ethernetMAC));
            return;
        }

        int isMACInvalid = validateMACAddress(eepromMAC, size);

        if (!isMACInvalid) {
            memcpy(netConfig->ethernetMAC, eepromMAC, size);
            printf("EEPROM MAC:\n");
            bufferDisplay(netConfig->ethernetMAC, size);
        }
        else {
            memcpy (netConfig->ethernetMAC, sysParamsNetConfig->ethernetMAC,
                    sizeof (netConfig->ethernetMAC));
            printf("EEPROM MAC is invalid! ASSIGNING DEFAULT:\n");
            bufferDisplay(netConfig->ethernetMAC, sizeof (netConfig->ethernetMAC));
        }
    }
}

/*
 * Print configuration routines
 */
void
showNetworkConfiguration(const struct sysNetParms *ipv4)
{
    printf("   IP ADDR: %s\n", formatIP(&ipv4->address, sizeof(ipv4->address)));
    printf("  NET MASK: %s\n", formatIP(&ipv4->netmask, sizeof(ipv4->netmask)));
    printf("   GATEWAY: %s\n", formatIP(&ipv4->gateway, sizeof(ipv4->gateway)));
}

void
systemParametersShowUserMGTrefClkOffsetPPM(void)
{
    printf("User MGT reference clock offset: %d PPM\n",
                                       systemParameters.userMGTrefClkOffsetPPM);
}
