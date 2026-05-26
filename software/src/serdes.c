#include <stdio.h>
#include <xil_io.h>
#include <lwip/def.h>

/*
 * Serializer/deserializers
 * Note -- Format routines share common static buffer.
 */
static char cbuf[40];

char *
formatMAC(const void *val, size_t size)
{
    (void) size;
    const unsigned char *addr = (const unsigned char *)val;
    snprintf(cbuf, sizeof(cbuf), "%02X:%02X:%02X:%02X:%02X:%02X",
            addr[0], addr[1], addr[2],
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
formatIP(const void *val, size_t size)
{
    (void) size;
    uint32_t l = ntohl(*(uint32_t *)val);
    snprintf(cbuf, sizeof(cbuf), "%d.%d.%d.%d", (int)(l >> 24) & 0xFF, (int)(l >> 16) & 0xFF,
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

char
*formatDouble(void *val, size_t size)
{
    (void) size;
    snprintf(cbuf, sizeof(cbuf), "%.15g", *(double *)val);
    return cbuf;
}

int
parseDouble(const char *str, void *val)
{
    char *endp;
    double d = strtod(str, &endp);
    if ((endp != str)
     && ((*endp == ',') || (*endp == '\r') || (*endp == '\n'))) {
        *(double *)val = d;
        return endp - str;
    }
    return -1;
}

char *
formatFloat(const void *val, size_t size)
{
    (void) size;
    snprintf(cbuf, sizeof(cbuf), "%.7g", *(const float *)val);
    return cbuf;
}

int
parseFloat(const char *str, void *val)
{
    int i;
    double d;

    i = parseDouble(str, &d);
    if (i > 0)
        *(float *)val = d;
    return i;
}

char *
formatInt(const void *val, size_t size)
{
    (void) size;
    snprintf(cbuf, sizeof(cbuf), "%d", *(const int *)val);
    return cbuf;
}

int
parseInt(const char *str, void *val)
{
    int i;
    double d;

    i = parseDouble(str, &d);
    if ((i > 0) && ((int)d == d))
        *(int *)val = d;
    return i;
}

char *
formatInt4(const void *val, size_t size)
{
    (void) size;
    snprintf(cbuf, sizeof(cbuf), "%04d", *(const int *)val);
    return cbuf;
}

int
parseHex(const char *str, void *val)
{
    char *endp;
    int d = strtol(str, &endp, 16);
    if ((endp != str)
     && ((*endp == ',') || (*endp == '\r') || (*endp == '\n'))) {
        *(unsigned long *)val = d;
        return endp - str;
    }
    return -1;
}

char *
formatHex(const void *val, size_t size)
{
    (void) size;
    snprintf(cbuf, sizeof(cbuf), "0x%x", *(const int *)val);
    return cbuf;
}

char *
formatNonTermString(const void *val, size_t size)
{
    const char *str = (const char *)val;
    snprintf(cbuf, sizeof(cbuf), "%.*s", (int) size, str);
    return cbuf;
}
