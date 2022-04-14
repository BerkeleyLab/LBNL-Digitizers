#include <stdio.h>
#include <lwipopts.h>
#include <lwip/init.h>
#include <lwip/inet.h>
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
    printf("\nFirmware POSIX seconds: %d\n", GPIO_READ(GPIO_IDX_FIRMWARE_BUILD_DATE));
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
    drawIPv4Address(&ipv4->address, isRecovery);

    /* Set up hardware */
    microsecondSpin(100000);
    sysmonInit();
    sfpChk();
    eyescanInit();
    microsecondSpin(100000);
    mgtInit();
    microsecondSpin(100000);
    evrInit();
    microsecondSpin(100000);
    rfClkInit();
    microsecondSpin(100000);
    rfClkShow();
    microsecondSpin(100000);
    mmcmInit();
    microsecondSpin(100000);
    sysrefInit();
    microsecondSpin(100000);
    sysrefShow();
    microsecondSpin(100000);
    rfADCinit();
    microsecondSpin(100000);
    afeInit();
    rfADCrestart();
    rfADCshow();
    rfADCsync();
    afeStart();

    /* Start network */
    lwip_init();
    printf("Network:\n");
    printf("       MAC: %s\n", formatMAC(enetMAC));
    showNetworkConfiguration(ipv4);
    ipaddr.addr = ipv4->address;
    netmask.addr = ipv4->netmask;
    gateway.addr = ipv4->gateway;
    printf("If things lock up at this point it's likely because\n"
           "the network driver can't negotiate a connection.\n");
    displayShowStatusLine("-- Intializing Network --");
    if (!xemac_add(&netif, &ipaddr, &netmask, &gateway, enetMAC,
                                                     XPAR_XEMACPS_0_BASEADDR)) {
        fatal("Error adding network interface");
    }
    netif_set_default(&netif);
    netif_set_up(&netif);
    displayShowStatusLine("");

    /* Set up communications and acquisition */
    epicsInit();
    tftpInit();
    acquisitionInit();
for (int i = 0 ; i < CFG_ADC_CHANNEL_COUNT ; i++) { afeSetGain(i, 16); }

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
