/*
 * Simple command interpreter
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <lwipopts.h>
#include <lwip/inet.h>
#include <lwip/udp.h>
#include <xparameters.h>
#include <xuartps_hw.h>
#include "acquisition.h"
#include "afe.h"
#include "display.h"
#include "evr.h"
#include "eyescan.h"
#include "ffs.h"
#include "frequencyMonitor.h"
#include "gpio.h"
#include "iic.h"
#include "mgt.h"
#include "mmcm.h"
#include "rfadc.h"
#include "rfclk.h"
#include "st7789v.h"
#include "sysmon.h"
#include "sysref.h"
#include "systemParameters.h"
#include "user_mgt_refclk.h"
#include "util.h"

enum consoleMode { consoleModeCommand,
                   consoleModeLogReplay,
                   consoleModeBootQuery,
                   consoleModeNetQuery,
                   consoleModeMacQuery };
static enum consoleMode consoleMode;

/*
 * UDP console support
 */
#define CONSOLE_UDP_PORT    50004
#define UDP_CONSOLE_BUF_SIZE    1400
static struct udpConsole {
    struct udp_pcb *pcb;
    char            obuf[UDP_CONSOLE_BUF_SIZE];
    int             outIndex;
    uint32_t        usAtFirstOutputCharacter;
    struct pbuf    *pbufIn;
    int             inIndex;
    ip_addr_t       fromAddr;
    uint16_t        fromPort;
} udpConsole;

static void
udpConsoleDrain(void)
{
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, udpConsole.outIndex, PBUF_RAM);
    if (p) {
        memcpy(p->payload, udpConsole.obuf, udpConsole.outIndex);
        udp_sendto(udpConsole.pcb, p, &udpConsole.fromAddr, udpConsole.fromPort);
        pbuf_free(p);
    }
    udpConsole.outIndex = 0;
}

/*
 * Convert <newline> to <carriage return><newline> so
 * we can use normal looking printf format strings.
 * Hang on to startup messages.
 * Buffer to limit the number of transmitted packets.
 * Called from LWIP statistics xil_printf so we have to insert carriage
 * returns ass necessary.
 */
#define STARTBUF_SIZE   120000
static char startBuf[STARTBUF_SIZE];
static int startIdx = 0;
static int isStartup = 1;

void
outbyte(char8 c)
{
    static int wasReturn;

    if ((c == '\n') && !wasReturn) outbyte('\r');
    wasReturn = (c == '\r');
    XUartPs_SendByte(STDOUT_BASEADDRESS, c);
    if (isStartup && (startIdx < STARTBUF_SIZE))
        startBuf[startIdx++] = c;
    if (udpConsole.fromPort) {
        if (udpConsole.outIndex == 0)
            udpConsole.usAtFirstOutputCharacter = MICROSECONDS_SINCE_BOOT();
        udpConsole.obuf[udpConsole.outIndex++] = c;
        if (udpConsole.outIndex >= UDP_CONSOLE_BUF_SIZE)
            udpConsoleDrain();
    }
}

static int
cmdLOG(int argc, char **argv)
{
    consoleMode = consoleModeLogReplay;
    return 0;
}
static int
cmdLOGactive(void)
{
    static int i;

    if (consoleMode == consoleModeLogReplay) {
        if (i < startIdx) {
            outbyte(startBuf[i++]);
            return 1;
        }
        else {
            i = 0;
            consoleMode = consoleModeCommand;
        }
    }
    return 0;
}

/*
 * boot command
 */
static void
bootQueryCallback(int argc, char **argv)
{
    if (argc == 1) {
        if (strcasecmp(argv[0], "Y") == 0) {
            if (udpConsole.fromPort) udpConsoleDrain();
            microsecondSpin(1000);
            resetFPGA();
            return;
        }
        if (strcasecmp(argv[0], "N") == 0) {
            consoleMode = consoleModeCommand;
            return;
        }
    }
    printf("Reboot FPGA (y or n)? ");
    fflush(stdout);
}
static int
cmdBOOT(int argc, char **argv)
{
    char *cp = NULL;

    consoleMode = consoleModeBootQuery;
    bootQueryCallback(0, &cp);
    return 0;
}

static int
cmdCAL(int argc, char **argv)
{
    char *endp;
    int d;

    for ( ; argc > 1 ; argc--, argv++) {
        if (strcmp(argv[1], "-t") == 0) {
            afeEnableTrainingTone(1);
        }
        else {
            d = strtol(argv[1], &endp, 0);
            if (*endp != '\0') {
                printf("Bad DAC value argument.\n");
            }
            else {
                if (d < 0) d = 0;
                if (d > 0xFFFF) d = 0xFFFF;
                printf("Set calibration DAC to 0x%x (%d)\n", d, d);
                afeSetDAC(d);
            }
        }
    }
    return 0;
}

static int
cmdDEBUG(int argc, char **argv)
{
    char *endp;
    int d;
    int sFlag = 0;

    if ((argc > 1) && (strcmp(argv[1], "-s") == 0)) {
        sFlag = 1;
        argc--;
        argv++;
    }
    if (argc > 1) {
        d = strtol(argv[1], &endp, 16);
        if (*endp == '\0') {
            debugFlags = d;
        }
    }
    printf("Debug flags: 0x%x\n", debugFlags);
    if (debugFlags & DEBUGFLAG_SHOW_TEST) st7789vTestPattern();
    if (debugFlags & DEBUGFLAG_ADC_MMCM_SHOW) mmcmShow();
    if (debugFlags & DEBUGFLAG_SHOW_DRP) eyescanCommand(0, NULL);
    if (debugFlags & DEBUGFLAG_RF_CLK_SHOW) rfClkShow();
    if (debugFlags & DEBUGFLAG_SHOW_AFE_REG) afeShow();
    if (debugFlags & DEBUGFLAG_RF_ADC_SHOW) rfADCshow();
    if (debugFlags & DEBUGFLAG_SHOW_SYSREF) sysrefShow();
    if (debugFlags & DEBUGFLAG_SLIDE_MGT) mgtRxBitslide();
    if (debugFlags & DEBUGFLAG_RESTART_AFE_ADC) afeADCrestart();
    if (debugFlags & DEBUGFLAG_RESYNC_ADC) rfADCsync();
    if (sFlag) {
        systemParameters.startupDebugFlags = debugFlags;
        systemParametersStash();
        printf("Startup debug flags: 0x%x\n", debugFlags);
    }
    return 0;
}

static int
cmdEVR(int argc, char **argv)
{
    evrShow();
    return 0;
}

static int
cmdFMON(int argc, char **argv)
{
    int i;
    int usingPPS = frequencyMonitorUsingPPS();
    static const char *names[] = { "System",
                                   "EVR recovered",
                                   "EVR Tx",
                                   "ADC AXI",
                                   "RFDC ADC0",
                                   "FPGA_REFCLK_OUT",
                                   "PRBS",
                                   "MGTref/2" };

    if (!usingPPS) {
        printf("Warning -- Pulse-per-second event not seen at power-up.\n");
        printf("           Frequency measurements are low accuracy.\n");
    }
    for (i = 0 ; i < sizeof names / sizeof names[0] ; i++) {
        printf("%16s clock:%*.*f\n", names[i], usingPPS ? 11 : 8,
                                               usingPPS ? 6 : 4,
                                               frequencyMonitorGet(i) / 1.0e6);
    }
    return 0;
}

/*
 * Network configuration
 */
static struct sysNetParms ipv4;
static void
netQueryCallback(int argc, char **argv)
{
    if (argc == 1) {
        if (strcasecmp(argv[0], "Y") == 0) {
            systemParameters.netConfig.ipv4 = ipv4;
            systemParametersStash();
            consoleMode = consoleModeCommand;
            return;
        }
        if (strcasecmp(argv[0], "N") == 0) {
            consoleMode = consoleModeCommand;
            return;
        }
    }
    showNetworkConfiguration(&ipv4);
    if ((consoleMode == consoleModeNetQuery)
     || (ipv4.address != systemParameters.netConfig.ipv4.address)
     || (ipv4.netmask != systemParameters.netConfig.ipv4.netmask)
     || (ipv4.gateway != systemParameters.netConfig.ipv4.gateway)) {
        printf("Write parameters to flash (y or n)? ");
        fflush(stdout);
        consoleMode = consoleModeNetQuery;
    }
}
static int
cmdNET(int argc, char **argv)
{
    int bad = 0;
    int i;
    char *cp;
    int netLen = 24;
    char *endp;

    if (argc == 1) {
        ipv4 = systemParameters.netConfig.ipv4;
    }
    else if (argc == 2) {
        cp = argv[1];
        i = parseIP(cp, &ipv4.address);
        if (i < 0) {
            bad = 1;
        }
        else if (cp[i] == '/') {
            netLen = strtol(cp + i + 1, &endp, 0);
            if ((*endp != '\0')
             || (netLen < 8)
             || (netLen > 24)) {
                bad = 1;
            }
        }
        ipv4.netmask = ~0 << (32 - netLen);
        ipv4.gateway = (ntohl(ipv4.address) & ipv4.netmask) | 1;
        ipv4.netmask = htonl(ipv4.netmask);
        ipv4.gateway = htonl(ipv4.gateway);
    }
    else {
        bad = 1;
    }
    if (bad) {
        printf("Command takes single optional argument of the form "
               "www.xxx.yyy.xxx[/n]\n");
        return 1;
    }
    cp = NULL;
    netQueryCallback(0, &cp);
    return 0;
}

/*
 * MAC configuration
 */
static unsigned char macBuf[6];
static void
macQueryCallback(int argc, char **argv)
{
    if (argc == 1) {
        if (strcasecmp(argv[0], "Y") == 0) {
            memcpy(systemParameters.netConfig.ethernetMAC,macBuf, sizeof macBuf);
            systemParametersStash();
            consoleMode = consoleModeCommand;
            return;
        }
        if (strcasecmp(argv[0], "N") == 0) {
            consoleMode = consoleModeCommand;
            return;
        }
    }
    printf("   ETHERNET ADDRESS: %s\n", formatMAC(&macBuf));
    if ((consoleMode == consoleModeMacQuery)
     || (memcmp(systemParameters.netConfig.ethernetMAC,macBuf,sizeof macBuf))) {
        printf("Write parameters to flash (y or n)? ");
        fflush(stdout);
        consoleMode = consoleModeMacQuery;
    }
}
static int
cmdMAC(int argc, char **argv)
{
    int bad = 0;
    int i;
    char *cp;

    if (argc == 1) {
        memcpy(macBuf, systemParameters.netConfig.ethernetMAC, sizeof macBuf);
    }
    else if (argc == 2) {
        i = parseMAC(argv[1], macBuf);
        if ((i < 0) || (argv[1][i] != '\0')) {
            bad = 1;
        }
    }
    else {
        bad = 1;
    }
    if (bad) {
        printf("Command takes single optional argument of the form "
               "aa:bb:cc:dd:ee:ff\n");
        return 1;
    }
    cp = NULL;
    macQueryCallback(0, &cp);
    return 0;
}

static int
cmdSTATS(int argc, char **argv)
{
    stats_display();
    return 0;
}

static int
cmdSYSMON(int argc, char **argv)
{
    sysmonDisplay();
    return 0;
}

static int
cmdREG(int argc, char **argv)
{
    char *endp;
    int i;
    int first;
    int n = 1;

    if (argc > 1) {
        first = strtol(argv[1], &endp, 0);
        if (*endp != '\0')
            return 1;
        if (argc > 2) {
            n = strtol(argv[2], &endp, 0);
            if (*endp != '\0')
                return 1;
        }
        if ((first < 0) || (first >= GPIO_IDX_COUNT) || (n <= 0))
            return 1;
        if ((first + n) > GPIO_IDX_COUNT)
            n = GPIO_IDX_COUNT - first;
        for (i = first ; i < first + n ; i++) {
            showReg(i);
        }
    }
    return 0;
}

static int
cmdTLOG(int argc, char **argv)
{
    uint32_t csr;
    static int isActive, isFirstHB, todBitCount;
    static int rAddr;
    static int addrMask;
    int pass = 0;

    if (argc < 0) {
        if (isActive) {
            GPIO_WRITE(GPIO_IDX_EVENT_LOG_CSR, 0);
            isActive = 0;
        }
        return 0;
    }
    if (argc > 0) {
        csr = GPIO_READ(GPIO_IDX_EVENT_LOG_CSR);
        addrMask = ~(~0UL << ((csr >> 24) & 0xF));
        GPIO_WRITE(GPIO_IDX_EVENT_LOG_CSR, 0x80000000);
        rAddr = 0;
        isActive = 1;
        isFirstHB = 1;
        todBitCount = 0;
        return 0;
    }
    if (isActive) {
        int wAddr, wAddrOld;
        static uint32_t lastHbTicks, lastEvTicks, todShift;
        csr = GPIO_READ(GPIO_IDX_EVENT_LOG_CSR);
        wAddrOld = csr & addrMask;
        for (;;) {
            csr = GPIO_READ(GPIO_IDX_EVENT_LOG_CSR);
            wAddr = csr & addrMask;
            if (wAddr == wAddrOld) break;
            if (++pass > 10) {
                printf("Event logger unstable!\n");
                isActive = 0;
                return 0;
            }
            wAddrOld = wAddr;
        }
        for (pass = 0 ; rAddr != wAddr ; ) {
            int event;
            GPIO_WRITE(GPIO_IDX_EVENT_LOG_CSR, 0x80000000 | rAddr);
            rAddr = (rAddr + 1) & addrMask;
            event = (GPIO_READ(GPIO_IDX_EVENT_LOG_CSR) >> 16) & 0xFF;
            if (event == 112) {
                todBitCount++;
                todShift = (todShift << 1) | 0;
            }
            else if (event == 113) {
                todBitCount++;
                todShift = (todShift << 1) | 1;
            }
            else {
                uint32_t ticks = GPIO_READ(GPIO_IDX_EVENT_LOG_TICKS);
                switch(event) {
                case 122: 
                    if (isFirstHB) {
                        printf("HB\n");
                        isFirstHB = 0;
                    }
                    else {
                        printf("HB %d\n", ticks - lastHbTicks);
                    }
                    lastHbTicks = ticks;
                    break;

                case 125:
                    if (todBitCount == 32) {
                        printf("PPS %d\n", todShift);
                    }
                    else {
                        printf("PPS\n");
                    }
                    todBitCount = 0;
                    break;

                default:
                    printf("%d %d\n", event, ticks - lastEvTicks);
                    lastEvTicks = ticks;
                    break;
                }
            }
            if (++pass >= addrMask) {
                printf("Event logger can't keep up.\n");
                isActive = 0;
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

static int
cmdUMGT(int argc, char **argv)
{
    if (argc > 1) {
        char *endp;
        int offsetPPM = strtol(argv[1], &endp, 10);
        if (*endp == '\0') {
            if (offsetPPM > 3500) offsetPPM = 3500;
            else if (offsetPPM < -3500) offsetPPM = -3500;
            if (userMGTrefClkAdjust(systemParameters.userMGTrefClkOffsetPPM)) {
                systemParameters.userMGTrefClkOffsetPPM = offsetPPM;
                systemParametersStash();
            }
        }
    }
    systemParametersShowUserMGTrefClkOffsetPPM();
    return 0;
}

struct commandInfo {
    const char *name;
    int       (*handler)(int argc, char **argv);
    const char *description;
};
static struct commandInfo commandTable[] = {
  { "boot",   cmdBOOT,  "Reboot FPGA"                        },
  { "cal"  ,  cmdCAL,   "Set calibration signals"            },
  { "DIR",    ffsShow,  "Show micro SD cards files"          },
  { "debug",  cmdDEBUG, "Set debug flags"                    },
  { "evr",    cmdEVR,   "Show EVR configuration"             },
  { "fmon"  , cmdFMON,  "Show clock frequencies"             },
  { "log",    cmdLOG,   "Replay startup console output"      },
  { "mac",    cmdMAC,   "Set Ethernet MAC address"           },
  { "net",    cmdNET,   "Set network parameters"             },
  { "reg",    cmdREG,   "Show GPIO register(s)"              },
  { "stats",  cmdSTATS, "Show network statistics"            },
  { "tlog",   cmdTLOG,  "Timing system event logger"         },
  { "userMGT",cmdUMGT,  "User MGT reference clock adjustment"},
  { "values", cmdSYSMON,"Show system monitor values"         },
  { "xcvr",   eyescanCommand,"Perform transceiver eye scan"  },
};
static void
commandCallback(int argc, char **argv)
{
    int i;
    int len;
    int matched = -1;

    if (argc <= 0)
        return;
    len = strlen(argv[0]);
    for (i = 0 ; i < sizeof commandTable / sizeof commandTable[0] ; i++) {
        if (strncmp(argv[0], commandTable[i].name, len) == 0) {
            if (matched >= 0) {
                printf("Not unique.\n");
                return;
            }
            matched = i;
        }
    }
    if (matched >= 0) {
        (*commandTable[matched].handler)(argc, argv);
        return;
    }
    if ((strncasecmp(argv[0], "help", len) == 0) || (argv[0][0] == '?')) {
        printf("Commands:\n");
        for (i = 0 ; i < sizeof commandTable / sizeof commandTable[0] ; i++) {
            printf("%8s -- %s\n", commandTable[i].name,
                                  commandTable[i].description);
        }
    }
    else {
        printf("Invalid command\n");
    }
}

static void
handleLine(char *line)
{
    char *argv[10];
    int argc;
    char *tokArg, *tokSave;

    argc = 0;
    tokArg = line;
    while ((argc < (sizeof argv / sizeof argv[0]) - 1)) {
        char *cp = strtok_r(tokArg, " ,", &tokSave);
        if (cp == NULL)
            break;
        argv[argc++] = cp;
        tokArg = NULL;
    }
    argv[argc] = NULL;
    switch (consoleMode) {
    case consoleModeCommand:   commandCallback(argc, argv);           break;
    case consoleModeLogReplay:                                        break;
    case consoleModeBootQuery: bootQueryCallback(argc, argv);         break;
    case consoleModeNetQuery:  netQueryCallback(argc, argv);          break;
    case consoleModeMacQuery:  macQueryCallback(argc, argv);          break;
    }
}

/*
 * Handle UDP console input
 */
void
console_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                                      const ip_addr_t *fromAddr, u16_t fromPort)
{
    /* Silently ignore runt packets and overruns */
    if ((p->len == 0) || (udpConsole.pbufIn != NULL)) {
        pbuf_free(p);
        return;
    }
    udpConsole.fromAddr = *fromAddr;
    udpConsole.fromPort = fromPort;
    udpConsole.pbufIn = p;
    udpConsole.inIndex = 0;
}

/*
 * Check for and act upon character from console
 */
void
consoleCheck(void)
{
    int c;
    static char line[200];
    static int idx = 0;
    static int firstTime = 1;

    if (firstTime) {
        int err;
        firstTime = 0;
        udpConsole.pcb = udp_new();
        if (udpConsole.pcb == NULL)
            fatal("Can't create console PCB");
        err = udp_bind(udpConsole.pcb, IP_ADDR_ANY, CONSOLE_UDP_PORT);
        if (err != ERR_OK)
            fatal("Can't bind to console port, error:%d", err);
        udp_recv(udpConsole.pcb, console_callback, NULL);
    }
    if (udpConsole.outIndex != 0) {
        if ((MICROSECONDS_SINCE_BOOT() - udpConsole.usAtFirstOutputCharacter) >
                                                                       100000) {
            udpConsoleDrain();
        }
    }
    if (cmdLOGactive()) return;
    if (eyescanCrank()) return;
    if (XUartPs_IsReceiveData(STDIN_BASEADDRESS)) {
        c = XUartPs_RecvByte(STDIN_BASEADDRESS) & 0xFF;
        udpConsole.fromPort = 0;
    }
    else if (udpConsole.pbufIn) {
        c = *((char *)udpConsole.pbufIn->payload + udpConsole.inIndex++);
        if (udpConsole.inIndex >= udpConsole.pbufIn->len) {
            pbuf_free(udpConsole.pbufIn);
            udpConsole.pbufIn = NULL;
        }
    }
    else {
        cmdTLOG(0, NULL);
        return;
    }
    cmdTLOG(-1, NULL);
    if ((c == '\001') || (c > '\177')) return;
    if (c == '\t') c = ' ';
    else if (c == '\177') c = '\b';
    else if (c == '\r') c = '\n';
    if (c == '\n') {
        isStartup = 0;
        printf("\n");
        line[idx] = '\0';
        idx = 0;
        handleLine(line);
        return;
    }
    if (c == '\b') {
        if (idx) {
            printf("\b \b");
            fflush(stdout);
            idx--;
        }
        return;
    }
    if (c < ' ')
        return;
    if (idx < ((sizeof line) - 1)) {
        printf("%c", c);
        fflush(stdout);
        line[idx++] = c;
    }
}
