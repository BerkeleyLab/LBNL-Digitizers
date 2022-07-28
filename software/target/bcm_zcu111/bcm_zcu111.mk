__BCM_ZCU111_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BCM_ZCU111_DIR := $(__BCM_ZCU111_DIR:/=)

__HDR_GEN_BCM_ZCU111_FILES = \
	lmk04208.h \
	lmx2594.h
HDR_GEN_BCM_ZCU111_FILES = $(addprefix $(BCM_ZCU111_DIR)/, $(__HDR_GEN_BCM_ZCU111_FILES))

__HDR_BCM_ZCU111_FILES = \
	iic.h \
	gpio.h \
	config.h
HDR_BCM_ZCU111_FILES = $(addprefix $(BCM_ZCU111_DIR)/, $(__HDR_BCM_ZCU111_FILES))

__SRC_BCM_ZCU111_FILES = \
	iic.c
SRC_BCM_ZCU111_FILES = $(addprefix $(BCM_ZCU111_DIR)/, $(__SRC_BCM_ZCU111_FILES))

# For top-level makfile
HDR_FILES += $(HDR_GEN_BCM_ZCU111_FILES) $(HDR_BCM_ZCU111_FILES)
SRC_FILES += $(SRC_BCM_ZCU111_FILES)

%lmk04208.h: %lmk04208.tcs
	sh $(SW_BCM_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

%lmx2594.h: %lmx2594.tcs
	sh $(SW_BCM_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

clean::
	$(RM) -rf $(HDR_GEN_BCM_ZCU111_FILES)
