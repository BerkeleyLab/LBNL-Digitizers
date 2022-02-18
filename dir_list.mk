TOP := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

GATEWARE_DIR       = $(TOP)gateware
SOFTWARE_DIR       = $(TOP)software

# Gateware

SUBMODULES_DIR     = $(GATEWARE_DIR)/ip_cores
MODULES_DIR        = $(GATEWARE_DIR)/modules
PLATFORM_DIR       = $(GATEWARE_DIR)/platform
GW_SCRIPTS_DIR     = $(GATEWARE_DIR)/scripts

BEDROCK_DIR        = $(SUBMODULES_DIR)/bedrock
EVR_DIR            = $(MODULES_DIR)/evr
PLATFORM_ZU28_DIR  = $(PLATFORM_DIR)/xilinx/zu28

GW_HSD_DIR         = $(GATEWARE_DIR)/syn/hsd_ref

# Sofware

SW_SCRIPTS_DIR     = $(SOFTWARE_DIR)/scripts

# HSD Sofware

SW_HSD_DIR         = $(SOFTWARE_DIR)/hsd

SW_HSD_SRC_DIR     = $(SW_HSD_DIR)/src
SW_HSD_SCRIPTS_DIR = $(SW_HSD_DIR)/scripts

include $(BEDROCK_DIR)/dir_list.mk
