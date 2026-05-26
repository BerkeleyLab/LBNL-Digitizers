/*
 * Common serializer/deserializer routines
 */

#ifndef _SERDES_H_
#define _SERDES_H_

#include <stdint.h>

char *formatIP(const void *val, size_t size);
int   parseIP(const char *str, void *val);

char *formatMAC(const void *val, size_t size);
int   parseMAC(const char *str, void *val);

char *formatIP(const void *val, size_t size);
int parseIP(const char *str, void *val);

char *formatDouble(void *val, size_t size);
int parseDouble(const char *str, void *val);

char *formatFloat(const void *val, size_t size);
int parseFloat(const char *str, void *val);

char *formatInt(const void *val, size_t size);
char *formatInt4(const void *val, size_t size);
int parseInt(const char *str, void *val);

char *formatHex(const void *val, size_t size);
int parseHex(const char *str, void *val);

char *formatNonTermString(const void *val, size_t size);

#endif
