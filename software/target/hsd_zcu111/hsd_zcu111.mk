__HSD_ZCU111_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
HSD_ZCU111_DIR := $(__HSD_ZCU111_DIR:/=)

__HDR_GEN_HSD_ZCU111_FILES = \
	lmk04208.h \
	lmx2594.h
HDR_GEN_HSD_ZCU111_FILES = $(addprefix $(HSD_ZCU111_DIR)/, $(__HDR_GEN_HSD_ZCU111_FILES))

__HDR_HSD_ZCU111_FILES = \
	iic.h \
	gpio.h \
	config.h
HDR_HSD_ZCU111_FILES = $(addprefix $(HSD_ZCU111_DIR)/, $(__HDR_HSD_ZCU111_FILES))

__SRC_HSD_ZCU111_FILES = \
	iic.c
SRC_HSD_ZCU111_FILES = $(addprefix $(HSD_ZCU111_DIR)/, $(__SRC_HSD_ZCU111_FILES))

# For top-level makfile
HDR_FILES += $(HDR_GEN_HSD_ZCU111_FILES) $(HDR_HSD_ZCU111_FILES)
SRC_FILES += $(SRC_HSD_ZCU111_FILES)

%lmk04208.h: %lmk04208.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

%lmx2594.h: %lmx2594.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

clean::
	$(RM) -rf $(HDR_GEN_HSD_ZCU111_FILES)
