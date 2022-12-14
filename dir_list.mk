TOP := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

GATEWARE_DIR       = $(TOP)gateware
SOFTWARE_DIR       = $(TOP)software

# Gateware

SUBMODULES_DIR     = $(GATEWARE_DIR)/submodules
MODULES_DIR        = $(GATEWARE_DIR)/modules
PLATFORM_DIR       = $(GATEWARE_DIR)/platform
GW_SCRIPTS_DIR     = $(GATEWARE_DIR)/scripts

BEDROCK_DIR        = $(SUBMODULES_DIR)/bedrock
EVR_DIR            = $(MODULES_DIR)/evr
PLATFORM_ZU28_DIR  = $(PLATFORM_DIR)/xilinx/zu28
PLATFORM_ZU28_HSD_DIR  = $(PLATFORM_ZU28_DIR)/hsd
PLATFORM_ZU48_DIR  = $(PLATFORM_DIR)/xilinx/zu48
PLATFORM_ZU48_HSD_DIR  = $(PLATFORM_ZU48_DIR)/hsd

GW_SYN_DIR         = $(GATEWARE_DIR)/syn

# Sofware

SW_LIBS_DIR        = $(SOFTWARE_DIR)/libs
SW_TGT_DIR         = $(SOFTWARE_DIR)/target
SW_SCRIPTS_DIR     = $(SOFTWARE_DIR)/scripts
SW_SRC_DIR     	   = $(SOFTWARE_DIR)/src
SW_APP_DIR         = $(SOFTWARE_DIR)/app

# HSD Sofware

SW_HSD_DIR         = $(SW_APP_DIR)/hsd
SW_HSD_SCRIPTS_DIR = $(SW_HSD_DIR)/scripts

# BCM Sofware

SW_BCM_DIR         = $(SW_APP_DIR)/bcm
SW_BCM_SCRIPTS_DIR = $(SW_BCM_DIR)/scripts

include $(BEDROCK_DIR)/dir_list.mk
