__ZCU111_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ZCU111_DIR := $(__ZCU111_DIR:/=)

__HDR_GEN_ZCU111_FILES = \
	lmk04208.h \
	lmx2594.h
HDR_GEN_ZCU111_FILES = $(addprefix $(ZCU111_DIR)/, $(__HDR_GEN_ZCU111_FILES))

__HDR_ZCU111_FILES = \
	iic.h
HDR_ZCU111_FILES = $(addprefix $(ZCU111_DIR)/, $(__HDR_ZCU111_FILES))

__SRC_ZCU111_FILES = \
	iic.c
SRC_ZCU111_FILES = $(addprefix $(ZCU111_DIR)/, $(__SRC_ZCU111_FILES))

# For top-level makfile
HDR_FILES += $(HDR_GEN_ZCU111_FILES) $(HDR_ZCU111_FILES)
SRC_FILES += $(SRC_ZCU111_FILES)

%lmk04208.h: %lmk04208.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

%lmx2594.h: %lmx2594.tcs
	sh $(SW_HSD_SCRIPTS_DIR)/createRFCLKheader.sh $< > $@

clean::
	$(RM) -rf $(HDR_GEN_ZCU111_FILES)
