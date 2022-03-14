include dir_list.mk

CROSS_COMPILE ?=

TARGET = zcu111
HSD_APP_NAME = hsd_$(TARGET)
HSD_SW_APP_NAME = HighSpeedDigitizer
HSD_BIT      = $(GW_HSD_TGT_DIR)/hsd_$(TARGET)_top.bit

GW_HSD_TGT_DIR = $(GW_HSD_DIR)/hsd_$(TARGET)

.PHONY: all hsd_bit hsd_sw

all: hsd_bit hsd_sw

hsd_bit:
	make -C $(GW_HSD_TGT_DIR) APP_NAME=$(HSD_APP_NAME) $(HSD_APP_NAME)_top.bit

hsd_sw:
	make -C $(SW_HSD_DIR) TARGET=$(TARGET) APP_NAME=$(HSD_SW_APP_NAME) BIT=$(HSD_BIT) all

clean:
	make -C $(GW_HSD_TGT_DIR) clean
	make -C $(SW_HSD_DIR) clean
	rm -f *.log *.jou
