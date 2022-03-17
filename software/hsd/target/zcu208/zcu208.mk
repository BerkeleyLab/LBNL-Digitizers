__ZCU208_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ZCU208_DIR := $(__ZCU208_DIR:/=)

__HDR_GEN_ZCU208_FILES = \
	lmk04828B.h \
	lmx2594.h
HDR_GEN_ZCU208_FILES = $(addprefix $(ZCU208_DIR)/, $(__HDR_GEN_ZCU208_FILES))

__HDR_ZCU208_FILES = \
	iic.h
HDR_ZCU208_FILES = $(addprefix $(ZCU208_DIR)/, $(__HDR_ZCU208_FILES))

__SRC_ZCU208_FILES = \
	iic.c
SRC_ZCU208_FILES = $(addprefix $(ZCU208_DIR)/, $(__SRC_ZCU208_FILES))

# For top-level makfile
HDR_FILES += $(HDR_GEN_ZCU208_FILES)
SRC_FILES += $(SRC_ZCU208_FILES)

%lmk04828B.h: %lmk04828B.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

%lmx2594.h: %lmx2594.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

clean::
	$(RM) -rf $(HDR_GEN_ZCU208_FILES)
