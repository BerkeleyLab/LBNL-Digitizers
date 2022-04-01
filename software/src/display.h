/*
 * Display support
 */
#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240

#define DISPLAY_ENABLE_SECONDS 1200

#define DISPLAY_MODE_STARTUP    0
#define DISPLAY_MODE_PAGES      1
#define DISPLAY_MODE_WARNING    2
#define DISPLAY_MODE_FATAL      3

void displayUpdate(void);
void displaySetMode(int mode);
int displayGetMode(void);
void drawIPv4Address(const void *ipv4address, int isRecoveryMode);

void displayShowFatal(const char *msg);
void displayShowWarning(const char *msg);
void displayShowStatusLine(const char *cp);

#endif  /* _DISPLAY_H_ */
