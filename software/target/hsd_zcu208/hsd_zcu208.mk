__HSD_ZCU208_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
HSD_ZCU208_DIR := $(__HSD_ZCU208_DIR:/=)

__HDR_GEN_HSD_ZCU208_FILES = \
	lmk04828B.h \
	lmx2594.h
HDR_GEN_HSD_ZCU208_FILES = $(addprefix $(HSD_ZCU208_DIR)/, $(__HDR_GEN_HSD_ZCU208_FILES))

__HDR_HSD_ZCU208_FILES = \
	iic.h \
	gpio.h \
	config.h
HDR_HSD_ZCU208_FILES = $(addprefix $(HSD_ZCU208_DIR)/, $(__HDR_HSD_ZCU208_FILES))

__SRC_HSD_ZCU208_FILES = \
	iic.c
SRC_HSD_ZCU208_FILES = $(addprefix $(HSD_ZCU208_DIR)/, $(__SRC_HSD_ZCU208_FILES))

# For top-level makfile
HDR_FILES += $(HDR_GEN_HSD_ZCU208_FILES)
SRC_FILES += $(SRC_HSD_ZCU208_FILES)

%lmk04828B.h: %lmk04828B.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

%lmx2594.h: %lmx2594.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

clean::
	$(RM) -rf $(HDR_GEN_HSD_ZCU208_FILES)
