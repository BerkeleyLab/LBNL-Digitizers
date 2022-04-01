include dir_list.mk

CROSS_COMPILE    ?=
TARGET           ?= zcu111
APP              ?= hsd

APP_NAME     = $(APP)_$(TARGET)
SW_APP_NAME  = $(APP)
GW_TGT_DIR   = $(GW_SYN_DIR)/$(APP_NAME)
BIT          = $(GW_TGT_DIR)/$(APP_NAME)_top.bit
SW_TGT_DIR   = $(SW_APP_DIR)/$(APP)

.PHONY: all bit sw

all: bit sw

bit:
	make -C $(GW_TGT_DIR) APP_NAME=$(APP_NAME) $(APP_NAME)_top.bit

sw:
	make -C $(SW_TGT_DIR) TARGET=$(TARGET) APP_NAME=$(SW_APP_NAME) BIT=$(BIT) all

clean:
	make -C $(GW_TGT_DIR) clean
	make -C $(SW_TGT_DIR) clean
	rm -f *.log *.jou
