/*
 * Per-application EPICS support
 */

#include <stdio.h>
#include <string.h>
#include "platform_config.h"
#include "hsdProtocol.h"
#include "bcmProtocol.h"
#include "bpmProtocol.h"
#include "acquisition.h"
#include "lossOfBeam.h"
#include "localOscillator.h"
#include "epicsApplicationCommands.h"
#include "evr.h"
#include "gpio.h"
#include "util.h"
#include "autotrim.h"

/*
 * Set the acquisition trigger events
 */
static void
selectTriggerEventAction(int eventNumber, unsigned int idx, int action)
{
    static unsigned char oldEventNumber[2];

    if (idx >= (sizeof oldEventNumber / sizeof oldEventNumber[0])) {
        return;
    }
    if (oldEventNumber[idx]) {
        evrRemoveEventAction(oldEventNumber[idx], action);
        oldEventNumber[idx] = 0;
    }
    if ((eventNumber > 0) && (eventNumber < 255)) {
        evrAddEventAction(eventNumber, action);
        oldEventNumber[idx] = eventNumber;
    }
}

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
            replyArgCount = acquisitionStatus(replyp->args, HSD_PROTOCOL_ARG_CAPACITY);
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

        case HSD_PROTOCOL_CMD_LONGOUT_LO_GENERIC:
            switch (idx) {
            // BCM commands
            case BCM_PROTOCOL_CMD_LONGOUT_ACQUISITION_SIZE:
                acquisitionSetSize(cmdp->args[0]);
                break;

            case BCM_PROTOCOL_CMD_LONGOUT_PASS_COUNT:
                acquisitionSetPassCount(cmdp->args[0]);
                break;

            case BCM_PROTOCOL_CMD_LONGOUT_EVENT_TRIGGER:
                selectTriggerEventAction(cmdp->args[0], 0, EVR_RAM_TRIGGER_2);
                break;

            case BCM_PROTOCOL_CMD_LONGOUT_EVENT_INJECTION:
                selectTriggerEventAction(cmdp->args[0], 1, EVR_RAM_TRIGGER_3);
                break;

            // BPM commands
            case BPM_PROTOCOL_CMD_LONGOUT_GENERIC_LOB_THRSH:
                lossOfBeamThreshold(-1, cmdp->args[0]);
                break;

            case BPM_PROTOCOL_CMD_LONGOUT_GENERIC_TBT_SUM_SHIFT:
                 sdAccumulateSetTbtSumShift(cmdp->args[0]);
                break;

            case BPM_PROTOCOL_CMD_LONGOUT_GENERIC_MT_SUM_SHIFT:
                sdAccumulateSetMtSumShift(cmdp->args[0]);
                break;

            case BPM_PROTOCOL_CMD_LONGOUT_GENERIC_BUTTON_DSP:
                localOscSetDspAlgorithm(cmdp->args[0]);
                break;

            case BPM_PROTOCOL_CMD_LONGOUT_GENERIC_AUTOTRIM_CTL:
                 autotrimEnable(cmdp->args[0]);
                break;

            case BPM_PROTOCOL_CMD_LONGOUT_GENERIC_AUTOTRIM_THRS:
                autotrimSetThreshold(cmdp->args[0]);
                break;

            case BPM_PROTOCOL_CMD_LONGOUT_GENERIC_AUTOTRIM_FILT_SHFT:
                autotrimSetFilterShift(cmdp->args[0]);
                break;

            default: return -1;
            }
            break;

        default: return -1;
        }
        break;

    default: return -1;
    }
    return replyArgCount;
}
