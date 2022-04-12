include dir_list.mk

CROSS_COMPILE    ?=
PLATFORM         ?= zcu111
APP              ?= hsd

TARGET       = $(APP)_$(PLATFORM)
GW_TGT_DIR   = $(GW_SYN_DIR)/$(TARGET)
BIT          = $(GW_TGT_DIR)/$(TARGET)_top.bit
SW_TGT_DIR   = $(SW_APP_DIR)/$(APP)

.PHONY: all bit sw

all: bit sw

bit:
	make -C $(GW_TGT_DIR) APP_NAME=$(TARGET) $(TARGET)_top.bit

sw:
	make -C $(SW_TGT_DIR) TARGET=$(TARGET) APP_NAME=$(TARGET) BIT=$(BIT) all

clean:
	make -C $(GW_TGT_DIR) clean
	make -C $(SW_TGT_DIR) clean
	rm -f *.log *.jou
