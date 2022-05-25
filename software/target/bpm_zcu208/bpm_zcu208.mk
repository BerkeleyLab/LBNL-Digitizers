__BPM_ZCU208_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
BPM_ZCU208_DIR := $(__BPM_ZCU208_DIR:/=)

__HDR_GEN_BPM_ZCU208_FILES = \
	lmk04828B.h \
	lmx2594ADC.h \
	lmx2594DAC.h
HDR_GEN_BPM_ZCU208_FILES = $(addprefix $(BPM_ZCU208_DIR)/, $(__HDR_GEN_BPM_ZCU208_FILES))

__HDR_BPM_ZCU208_FILES = \
	iic.h \
	gpio.h \
	config.h
HDR_BPM_ZCU208_FILES = $(addprefix $(BPM_ZCU208_DIR)/, $(__HDR_BPM_ZCU208_FILES))

__SRC_BPM_ZCU208_FILES = \
	iic.c
SRC_BPM_ZCU208_FILES = $(addprefix $(BPM_ZCU208_DIR)/, $(__SRC_BPM_ZCU208_FILES))

# For top-level makfile
HDR_FILES += $(HDR_GEN_BPM_ZCU208_FILES)
SRC_FILES += $(SRC_BPM_ZCU208_FILES)

%lmk04828B.h: %lmk04828B.tcs
	sh $(SW_BPM_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

%lmx2594ADC.h: %lmx2594ADC.tcs
	sh $(SW_BPM_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

%lmx2594DAC.h: %lmx2594DAC.tcs
	sh $(SW_BPM_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

clean::
	$(RM) -rf $(HDR_GEN_BPM_ZCU208_FILES)
