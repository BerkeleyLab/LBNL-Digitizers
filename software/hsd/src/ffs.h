/*
 * Micro SD card file system
 */

#ifndef _FFS_H_
#define _FFS_H_

#include <ff.h>

void ffsCheck(void);
int ffsShow(int argc, char **argv);
const char *ffsStrerror(FRESULT fr);

#endif /* _FFS_H_ */
