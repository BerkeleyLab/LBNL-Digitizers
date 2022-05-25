/*
 *  Trivial File Transfer Protocol
 *
 * Strictly speaking TFTP is limited to 32 MB, but this server will transfer
 * larger files with clients that wrap around from block 65535 to block 0.
 */

#include <stdio.h>
#include <string.h>
#include <lwip/udp.h>
#include "afe.h"
#include "ffs.h"
#include "st7789v.h"
#include "tftp.h"
#include "util.h"

#define TFTP_PORT 69

#define TFTP_OPCODE_RRQ   1
#define TFTP_OPCODE_WRQ   2
#define TFTP_OPCODE_DATA  3
#define TFTP_OPCODE_ACK   4
#define TFTP_OPCODE_ERROR 5

#define TFTP_ERROR_ACCESS_VIOLATION 2

/*
 * Send an error reply
 */
static void
replyERR(struct udp_pcb *pcb, const ip_addr_t *fromAddr, u16_t fromPort,
         const char *msg)
{
    int l = (2 * sizeof (u16_t)) + strlen(msg) + 1;
    struct pbuf *p;
    u16_t *p16;

    p = pbuf_alloc(PBUF_TRANSPORT, l, PBUF_RAM);
    if (p == NULL) {
        printf("Can't allocate TFTP ERROR pbuf\n");
        return;
    }
    p16 = (u16_t *)p->payload;
    *p16++ = htons(TFTP_OPCODE_ERROR);
    *p16++ = htons(TFTP_ERROR_ACCESS_VIOLATION);
    strcpy((char *)p16, msg);
    udp_sendto(pcb, p, fromAddr, fromPort);
    pbuf_free(p);
}

/*
 * Send a success reply
 */
static void
replyACK(struct udp_pcb *pcb, const ip_addr_t *fromAddr, u16_t fromPort,
         int block)
{
    int l = 2 * sizeof (u16_t);
    struct pbuf *p;
    u16_t *p16;

    p = pbuf_alloc(PBUF_TRANSPORT, l, PBUF_RAM);
    if (p == NULL) {
        printf("Can't allocte TFTP ACK pbuf\n");
        return;
    }
    p16 = (u16_t*)p->payload;
    *p16++ = htons(TFTP_OPCODE_ACK);
    *p16++ = htons(block);
    udp_sendto(pcb, p, fromAddr, fromPort);
    pbuf_free(p);
}

/*
 * Send a data packet
 */
static int
sendBlock(struct udp_pcb *pcb, const ip_addr_t *fromAddr, u16_t fromPort,
                                                             int block, FIL *fp)
{
    int n;
    unsigned int l;
    struct pbuf *p;
    u16_t *p16;
    static char cbuf[512];
    static int lastCount;

    if (fp == NULL) {
        n = lastCount;
    }
    else {
        UINT nRead;
        FRESULT fr = f_read(fp, cbuf, sizeof cbuf, &nRead);
        if (fr != FR_OK) {
            replyERR(pcb, fromAddr, fromPort, ffsStrerror(fr));
            return 0;
        }
        n = nRead;
    }
    lastCount = n;
    l = (2 * sizeof(u16_t)) + n;
    p = pbuf_alloc(PBUF_TRANSPORT, l, PBUF_RAM);
    if (p == NULL) {
        printf("Can't allocte TFTP DATA pbuf\n");
        return 0;
    }
    p16 = (u16_t*)p->payload;
    *p16++ = htons(TFTP_OPCODE_DATA);
    *p16++ = htons(block);
    memcpy(p16, cbuf, n);
    udp_sendto(pcb, p, fromAddr, fromPort);
    pbuf_free(p);
    return n;
}

/*
 * Handle an incoming packet
 */
static void
tftp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                   const ip_addr_t *fromAddr, u16_t fromPort)
{
    unsigned char *cp = p->payload;
    long addr = htonl(fromAddr->addr);
    int ackBlock = -1;
    static u16_t lastBlock;
    static int lastSend;
    FRESULT fr;
    static FIL fil, *fp;
    static char stashAFE;

    if (debugFlags & DEBUGFLAG_TFTP)
        printf("%3d on port %d from %d.%d.%d.%d:%d  %02X%02X %02X%02X\n",
                                                    p->len,
                                                    pcb->local_port,
                                                    (int)((addr >> 24) & 0xFF),
                                                    (int)((addr >> 16) & 0xFF),
                                                    (int)((addr >>  8) & 0xFF),
                                                    (int)((addr      ) & 0xFF),
                                                    fromPort,
                                                    cp[0], cp[1], cp[2], cp[3]);
    /*
     * Ignore runt packets
     */
    if (p->len >= 4) {
        int opcode = (cp[0] << 8) | cp[1];
        if ((opcode == TFTP_OPCODE_RRQ)
         || (opcode == TFTP_OPCODE_WRQ)) {
            char *name = (char *)cp + 2, *mode = NULL;
            int nullCount = 0, i = 2;
            lastBlock = 0;
            while (i < p->len) {
                if (cp[i++] == '\0') {
                    nullCount++;
                    if (nullCount == 1)
                        mode = (char *)cp + i;
                    if (nullCount == 2) {
                        if (debugFlags & DEBUGFLAG_TFTP)
                            printf("NAME:%s  MODE:%s\n", name, mode);
                        if (strcasecmp(mode, "octet") != 0) {
                            replyERR(pcb, fromAddr, fromPort, "Bad Type");
                        }
                        else {
                            stashAFE = 0;
                            if (strcasecmp(name, "SCREEN.ppm") == 0) {
                                st7789vGrabScreen();
                            }
                            if (strcasecmp(name, AFE_EEPROM_NAME) == 0) {
                                if (opcode == TFTP_OPCODE_RRQ) {
                                    afeFetchEEPROM();
                                }
                                else {
                                    stashAFE = 1;
                                }
                            }
                            if (fp) {
                                f_close(fp);
                                fp = NULL;
                            }
                            fr = f_open(&fil, name, (opcode==TFTP_OPCODE_RRQ) ?
                                                   FA_READ :
                                                   FA_WRITE | FA_CREATE_ALWAYS);
                            if (fr == FR_OK) {
                                ackBlock = 0;
                                fp = &fil;
                            }
                            else {
                                const char *msg = ffsStrerror(fr);
                                if (debugFlags & DEBUGFLAG_TFTP) {
                                    printf("\"%s\" -- %s\n", name, msg);
                                }
                                replyERR(pcb, fromAddr, fromPort, msg);
                            }
                        }
                        break;
                    }
                }
            }
            if ((ackBlock == 0) && (opcode == TFTP_OPCODE_RRQ)) {
                ackBlock = -1;
                lastBlock = 1;
                lastSend = sendBlock(pcb, fromAddr, fromPort, lastBlock, fp);
            }
        }
        else if (opcode == TFTP_OPCODE_DATA) {
            int block = (cp[2] << 8) | cp[3];
            if (block == lastBlock) {
                ackBlock = block;
            }
            else if (block == (u16_t)(lastBlock + 1)) {
                int nBytes = p->len - (2 * sizeof(u16_t));
                lastBlock = block;
                ackBlock = block;
                if (nBytes > 0) {
                    UINT nWritten;
                    fr = f_write(fp, (char *)(cp+4), nBytes, &nWritten);
                    if ((fr != FR_OK) || (nWritten != nBytes)) {
                        ackBlock = -1;
                        if (debugFlags & DEBUGFLAG_TFTP) {
                            printf("Write failed %s %d!=%d\n", ffsStrerror(fr),
                                                         (int)nWritten, nBytes);
                        }
                        replyERR(pcb, fromAddr, fromPort, ffsStrerror(fr));
                    }
                }
                if (fp && (nBytes < 512)) { 
                    fr = f_close(fp);
                    fp = NULL;
                    if (fr != FR_OK) {
                        ackBlock = -1;
                        if (debugFlags & DEBUGFLAG_TFTP) {
                            printf("Close failed -- %s\n", ffsStrerror(fr));
                        }
                        replyERR(pcb, fromAddr, fromPort, ffsStrerror(fr));
                    }
                    if (stashAFE) {
                        stashAFE = 0;
                        afeStashEEPROM();
                    }
                }
            }
        }
        else if (opcode == TFTP_OPCODE_ACK) {
            int block = (cp[2] << 8) | cp[3];
            if (block == lastBlock) {
                lastBlock++;
                if (lastSend == 512) {
                    lastSend = sendBlock(pcb, fromAddr, fromPort, lastBlock, fp);
                }
                else {
                    f_close(fp);
                    fp = NULL;
                }
            }
            else {
                lastSend = sendBlock(pcb, fromAddr, fromPort, lastBlock, NULL);
            }
        }
    }
    pbuf_free(p);
    if (ackBlock >= 0)
        replyACK(pcb, fromAddr, fromPort, ackBlock);
}

/*
 * Set up UDP port and register for callbacks
 */
void
tftpInit(void)
{
    struct udp_pcb *pcb;
    int err;

    pcb = udp_new();
    if (pcb == NULL) {
        printf("Can't create TFTP PCB.\n");
        return;
    }
    err = udp_bind(pcb, IP_ADDR_ANY, TFTP_PORT);
    if (err != ERR_OK) {
        printf("Can't bind, error:%d\n", err);
        return;
    }
    udp_recv(pcb, tftp_recv_callback, NULL);
}
