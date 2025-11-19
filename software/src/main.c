#include <stdio.h>
#include <lwipopts.h>
#include <lwip/init.h>
#include <lwip/inet.h>
#if LWIP_DHCP==1
#include <lwip/dhcp.h>
#endif
#include <netif/xadapter.h>
#include "acquisition.h"
#include "afe.h"
#include "console.h"
#include "display.h"
#include "epics.h"
#include "evr.h"
#include "eyescan.h"
#include "ffs.h"
#include "gpio.h"
#include "iic.h"
#include "mgt.h"
#include "mmcm.h"
#include "platform.h"
#include "rfadc.h"
#include "rfclk.h"
#include "softwareBuildDate.h"
#include "st7789v.h"
#include "sysref.h"
#include "sysmon.h"
#include "systemParameters.h"
#include "tftp.h"
#include "util.h"

static void
sfpString(const char *name, int offset)
{
    uint8_t rxBuf[17];
    rxBuf[16] = '\0';
    if (iicRead (IIC_INDEX_SFP_0_INFO, offset, rxBuf, 16))
        printf("%16s: %s\n", name, rxBuf);
}
static void sfpChk(void)
{
    uint8_t rxBuf[10];
    int i;
    int f;
    sfpString("Manufacturer ID", 20);
    sfpString("Part Number", 40);
    sfpString("Serial Number", 68);
    if (iicRead (IIC_INDEX_SFP_0_INFO, 84, rxBuf, 6))
        printf("%16s: 20%c%c-%c%c-%c%c\n", "Date Code", rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4], rxBuf[5]);
    if (iicRead (IIC_INDEX_SFP_0_STATUS, 96, rxBuf, 10)) {
        i = rxBuf[0];
        f = (rxBuf[1] * 100) / 256;
        printf("     Temperature: %d.%02d\n", i, f);
        i = (rxBuf[2] << 8) + rxBuf[3];
        i /= 10;
        f = i % 1000;
        i /= 1000;
        printf("             Vcc: %d.%03d\n", i, f);
        i = (rxBuf[8] << 8) + rxBuf[9];
        f = i % 10;
        i /= 10;
        printf("   Rx Power (uW): %d.%d\n", i, f);
    }
}

#if LWIP_DHCP==1
extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);
#endif

static int
tryRequestDHCPIPv4Address(struct netif *netif)
{
#if LWIP_DHCP==1
    /*
     * Create a new DHCP client for this interface.
     * Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
     * the predefined regular intervals after starting the client.
     */
    dhcp_start(netif);

    /*
     * Reset timeout counter. This is defined in platform_zynqmp.c
     */
    dhcp_timoutcntr = 240;

    while (((netif->ip_addr.addr) == 0) && (dhcp_timoutcntr > 0)) {
        xemacif_input(netif);
    }

    if (dhcp_timoutcntr <= 0) {
        if ((netif->ip_addr.addr) == 0) {
            // Timeout
            IP4_ADDR(&(netif->ip_addr), 192, 168,   1, 10);
            IP4_ADDR(&(netif->netmask), 255, 255, 255,  0);
            IP4_ADDR(&(netif->gw),      192, 168,   1,  1);
            return -1;
        }
    }

    return 0;
#else
    return 0;
#endif
}

int
main(void)
{
    int isRecovery;
    unsigned char *enetMAC;
    const struct sysNetParms *ipv4;
    static ip_addr_t ipaddr, netmask, gateway;
    static struct netif netif;

    /* Set up infrastructure */
    init_platform();
    platform_enable_interrupts();
    isRecovery = resetRecoverySwitchPressed();

    /* Announce our presence */
    st7789vInit();
    printf("\nGit ID (32-bit): 0x%08x\n", GPIO_READ(GPIO_IDX_GITHASH));
    printf("Firmware POSIX seconds: %d\n", GPIO_READ(GPIO_IDX_FIRMWARE_BUILD_DATE));
    printf("Software POSIX seconds: %d\n", SOFTWARE_BUILD_DATE);

    /* Set up file system */
    ffsCheck();

    /* Get configuration settings */
    iicInit();
    systemParametersReadback();
    if (isRecovery) {
        printf("==== Recovery mode -- Using default network parameters ====\n");
        enetMAC = netDefault.ethernetMAC;
        ipv4 = &netDefault.ipv4;
    }
    else {
        enetMAC = systemParameters.netConfig.ethernetMAC;
        ipv4 = &systemParameters.netConfig.ipv4;
    }

    /*
     * Set default IPv4 configs based on system parameters file and
     * DHCP
     */
    setDefaultIPv4Address(&currentNetConfig,
            &systemParameters.netConfig,
            &netDefault, isRecovery);
    drawIPv4Address(&currentNetConfig.ipv4.address, isRecovery);

    /* Set up hardware */
    sysmonInit();
    sfpChk();
    eyescanInit();
    mgtInit();
    evrInit();
    rfClkInit();
    rfClkShow();
    mmcmInit();
    sysrefInit();
    sysrefShow();
    rfADCinit();
    afeInit();
    rfADCrestart();
    rfADCshow();
    rfADCsync();
    afeStart();

    /* Start network */
    lwip_init();
    ipaddr.addr = currentNetConfig.ipv4.address;
    netmask.addr = currentNetConfig.ipv4.netmask;
    gateway.addr = currentNetConfig.ipv4.gateway;
    printf("If things lock up at this point it's likely because\n"
           "the network driver can't negotiate a connection.\n");
    displayShowStatusLine("-- Intializing Network --");
    if (!xemac_add(&netif, &ipaddr, &netmask, &gateway, currentNetConfig.ethernetMAC,
                                                     XPAR_XEMACPS_0_BASEADDR)) {
        fatal("Error adding network interface");
    }
    netif_set_default(&netif);
    netif_set_up(&netif);

    /*
     * Try to request IP from DHCP. Could fail if timeout.
     * If DHCP is disabled, returns success
     */
    int ret = tryRequestDHCPIPv4Address(&netif);
    if (ret < 0) {
        warn("DHCP timeout. Default IP address assigned");
    }

    /*
     * Copy possibly changed IPv4 parameters by DHCP into out copy
     */
    ipaddr.addr = netif.ip_addr.addr;
    netmask.addr = netif.netmask.addr;
    gateway.addr = netif.gw.addr;
    currentNetConfig.ipv4.address = netif.ip_addr.addr;
    currentNetConfig.ipv4.netmask = netif.netmask.addr;
    currentNetConfig.ipv4.gateway = netif.gw.addr;

    printf("Network:\n");
    printf("       MAC: %s\n", formatMAC(currentNetConfig.ethernetMAC));
    showNetworkConfiguration(&currentNetConfig.ipv4);
    displayShowStatusLine("");

    /* Set up communications and acquisition */
    epicsInit();
    tftpInit();
    acquisitionInit();
    for (int i = 0 ; i < CFG_ADC_PHYSICAL_COUNT ; i++) { afeSetGain(i, 16); }

    /*
     * Main processing loop
     */
    printf("DFE serial number: %03d\n", serialNumberDFE());
    printf("AFE serial number: %03d\n", afeGetSerialNumber());
    if (displayGetMode() == DISPLAY_MODE_STARTUP) {
        displaySetMode(DISPLAY_MODE_PAGES);
    }
    for (;;) {
        checkForReset();
        acquisitionCrank();
        mgtCrankRxAligner();
        xemacif_input(&netif);
        consoleCheck();
        ffsCheck();
        displayUpdate();
    }

    /* Never reached */
    cleanup_platform();
    return 0;
}
