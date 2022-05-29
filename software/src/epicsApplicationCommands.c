/*
 * Per-application EPICS support
 */

#include <stdio.h>
#include <string.h>
#include "platform_config.h"
#include "hsdProtocol.h"
#include "acquisition.h"
#include "epicsApplicationCommands.h"
#include "gpio.h"
#include "util.h"

int
epicsApplicationCommand(int commandArgCount, struct hsdPacket *cmdp,
                                             struct hsdPacket *replyp)
{
    int lo = cmdp->command & HSD_PROTOCOL_CMD_MASK_LO;
    int idx = cmdp->command & HSD_PROTOCOL_CMD_MASK_IDX;
    int replyArgCount = 0;

    switch (cmdp->command & HSD_PROTOCOL_CMD_MASK_HI) {
    case HSD_PROTOCOL_CMD_HI_LONGIN:
        if (commandArgCount != 0) return -1;
        replyArgCount = 1;
        switch (idx) {
        case HSD_PROTOCOL_CMD_LONGIN_IDX_ACQ_STATUS:
            replyp->args[0] = acquisitionStatus();
            break;

        default: return -1;
        }
        break;

    case HSD_PROTOCOL_CMD_HI_LONGOUT:
        if (commandArgCount != 1) return -1;
        switch (lo) {
        case HSD_PROTOCOL_CMD_LONGOUT_LO_NO_VALUE:
            switch (idx) {
            case HSD_PROTOCOL_CMD_LONGOUT_NV_IDX_SOFT_TRIGGER:
                GPIO_WRITE(GPIO_IDX_SOFT_TRIGGER, 0);
                break;
            default: return -1;
            }
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_SET_PRETRIGGER_SAMPLES:
            acquisitionSetPretriggerCount(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_SET_INPUT_BONDING:
            acquisitionSetBonding(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_SET_TRIGGER_EDGE:
            acquisitionSetTriggerEdge(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_TRIGGER_LEVEL:
            acquisitionSetTriggerLevel(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_SET_TRIGGER_ENABLES:
            acquisitionSetTriggerEnables(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_REARM:
            acquisitionArm(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_SET_SEGMENTED_MODE:
            acquisitionSetSegmentedMode(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_EARLY_SEGMENT_INTERVAL:
            acquisitionSetEarlySegmentInterval(idx, cmdp->args[0]);
            break;

        case HSD_PROTOCOL_CMD_LONGOUT_LO_LATER_SEGMENT_INTERVAL:
            acquisitionSetLaterSegmentInterval(idx, cmdp->args[0]);
            break;

        default: return -1;
        }
        break;
    default: return -1;
    }
    return replyArgCount;
}
