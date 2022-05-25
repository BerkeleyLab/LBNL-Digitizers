#include <stdio.h>
#include "interlock.h"
#include "gpio.h"
#include "util.h"

#define CSR_INTERLOCK_RESET_BUTTON  0x1
#define CSR_INTERLOCK_RELAY_CONTROL 0x2
#define CSR_INTERLOCK_RELAY_OPEN    0x4
#define CSR_INTERLOCK_RELAY_CLOSED  0x8
#define CSR_INTERLOCK_RELAY_FAULT   0x10

int
interlockRelayState()
{
    uint32_t csr = GPIO_READ(GPIO_IDX_INTERLOCK_CSR);
    int open = ((csr & CSR_INTERLOCK_RELAY_OPEN) != 0);
    int closed = ((csr & CSR_INTERLOCK_RELAY_CLOSED) != 0);
    int control = ((csr & CSR_INTERLOCK_RELAY_CONTROL) != 0);

    if (open && closed) return INTERLOCK_STATE_FAULT_BOTH_ON;
    if (!open && !closed) return INTERLOCK_STATE_FAULT_BOTH_OFF;
    if (control && !closed) return INTERLOCK_STATE_FAULT_NOT_CLOSED;
    if (!control && !open) return INTERLOCK_STATE_FAULT_NOT_OPEN;
    if (closed) return INTERLOCK_STATE_RELAY_CLOSED;
    return INTERLOCK_STATE_RELAY_OPEN;
}
