/*
 * FAT file system support
 */
#include <stdio.h>
#include <string.h>
#include <diskio.h>
#include "gpio.h"
#include "ffs.h"
#include "util.h"

#define SCAN_BUF_SIZE   256

/*
 * Show filesystem contents
 * Based on FFS example code
 */
static FRESULT
scan_files (char *path)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;
    static int depth;

    if (depth > 10) {
        printf("=== too many nested directories\n");
        return FR_OK;
    }
    depth++;
    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);
            if ((res != FR_OK) || (fno.fname[0] == 0)) break;
            if (fno.fattrib & AM_DIR) {
                i = strlen(path);
                if ((i + strlen(fno.fname) + 2) > SCAN_BUF_SIZE) {
                    printf("=== name too long\n");
                    break;
                }
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);
                path[i] = 0;
                if (res != FR_OK) break;
            }
            else {
                printf("%9lu %s/%s\n", (unsigned long)fno.fsize, path, fno.fname);
            }
        }
        f_closedir(&dir);
    }
    depth--;
    return res;
}

int
ffsShow(int argc, char **argv) {
    FRESULT fr;
    static char cbuf[SCAN_BUF_SIZE];

    cbuf[0] = 0;
    fr = scan_files(cbuf);
    return (fr != FR_OK);
}

static void
umount(void)
{
    FRESULT fr;
    if ((fr = f_mount(NULL, "", 0)) != FR_OK) {
        fatal("f_umount: %s", ffsStrerror(fr));
    }
}

void
ffsCheck(void)
{
    uint32_t now = MICROSECONDS_SINCE_BOOT();
    DSTATUS ds;
    FRESULT fr;
    static FATFS fs;
    static int isMounted = 0;
    static int firstTime = 1;
    static uint32_t whenChecked;

    if (firstTime || ((now - whenChecked) >= 1000000)) {
        firstTime = 0;
        whenChecked = now;
        ds = disk_status(0);
        if (ds & STA_NODISK) {
            if (isMounted) {
                umount();
                isMounted = 0;
            }
        }
        else {
            if ((ds & STA_NOINIT) || !isMounted) {
                if (isMounted) umount();
                if ((fr = f_mount(&fs, "", 1)) == FR_OK) {
                    printf("Micro SD card mounted.\n");
                    isMounted = 1;
                }
                else {
                    printf("Mount failed: %s\n", ffsStrerror(fr));
                    isMounted = 0;
                }
            }
        }
    }
}

const char *
ffsStrerror(FRESULT fr)
{
    switch (fr) {
    case FR_OK:                  return "FR_OK";
    case FR_DISK_ERR:            return "FR_DISK_ERR";
    case FR_INT_ERR:             return "FR_INT_ERR";
    case FR_NOT_READY:           return "FR_NOT_READY";
    case FR_NO_FILE:             return "FR_NO_FILE";
    case FR_NO_PATH:             return "FR_NO_PATH";
    case FR_INVALID_NAME:        return "FR_INVALID_NAME";
    case FR_DENIED:              return "FR_DENIED";
    case FR_EXIST:               return "FR_EXIST";
    case FR_INVALID_OBJECT:      return "FR_INVALID_OBJECT";
    case FR_WRITE_PROTECTED:     return "FR_WRITE_PROTECTED";
    case FR_INVALID_DRIVE:       return "FR_INVALID_DRIVE";
    case FR_NOT_ENABLED:         return "FR_NOT_ENABLED";
    case FR_NO_FILESYSTEM:       return "FR_NO_FILESYSTEM";
    case FR_MKFS_ABORTED:        return "FR_MKFS_ABORTED";
    case FR_TIMEOUT:             return "FR_TIMEOUT";
    case FR_LOCKED:              return "FR_LOCKED";
    case FR_NOT_ENOUGH_CORE:     return "FR_NOT_ENOUGH_CORE";
    case FR_TOO_MANY_OPEN_FILES: return "FR_TOO_MANY_OPEN_FILES";
    case FR_INVALID_PARAMETER:   return "FR_INVALID_PARAMETER";
    default: return "??";
    }
}
