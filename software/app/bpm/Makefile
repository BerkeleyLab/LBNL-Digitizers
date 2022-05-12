__THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
THIS_DIR := $(__THIS_DIR:/=)

CROSS_COMPILE ?= aarch64-none-elf-
CC = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
XSCT ?= xsct -norlwrap

include ../../../dir_list.mk

TARGET   ?= bpm_zcu208
GW_BPM_TGT_DIR = $(GW_SYN_DIR)/$(TARGET)
BIT      ?= $(GW_BPM_TGT_DIR)/$(TARGET)_top.bit

BIF   = $(SW_BPM_SCRIPTS_DIR)/BOOTGEN.bif
ELF   = $(THIS_DIR)/$(TARGET).elf
PMUFW = $(THIS_DIR)/$(TARGET)/platform/export/platform/sw/platform/boot/pmufw.elf
BIN   = $(THIS_DIR)/BOOT.bin

# Vivado 2020.1 uses executable.elf name and Vivado 2020.2 uses fsbl_a53.elf
FSBL  = $(THIS_DIR)/$(TARGET)/platform/zynqmp_fsbl/fsbl_a53.elf

TARGET_DIR = $(SW_TGT_DIR)/$(TARGET)

all: $(TARGET) $(TARGET).elf $(TARGET).elf.size $(BIN)

__SRC_FILES = \
	acquisition.c \
	acqSync.c \
	afe.c \
	console.c \
	display.c \
	epics.c \
	epicsApplicationCommands.c \
	evr.c \
	eyescan.c \
	ffs.c \
	frequencyMonitor.c \
	interlock.c \
	main.c \
	mgt.c \
	mmcm.c \
	platform_zynqmp.c \
	rfadc.c \
	rfclk.c \
	sysmon.c \
	sysref.c \
	systemParameters.c \
	tftp.c \
	localOscillator.c \
	user_mgt_refclk.c \
	util.c
SRC_FILES = $(addprefix $(SW_SRC_DIR)/, $(__SRC_FILES))

__HDR_FILES = \
	acquisition.h \
	acqSync.h \
	afe.h \
	console.h \
	display.h \
	epics.h \
	epicsApplicationCommands.h \
	evr.h \
	eyescan.h \
	ffs.h \
	frequencyMonitor.h \
	hsdProtocol.h \
	interlock.h \
	lv_font.h \
	mgt.h \
	mmcm.h \
	platform.h \
	platform_config.h \
	rfadc.h \
	rfclk.h \
	sysmon.h \
	sysref.h \
	systemParameters.h \
	tftp.h \
	localOscillator.h \
	loTables.h \
	user_mgt_refclk.h \
	util.h
HDR_FILES = $(addprefix $(SW_SRC_DIR)/, $(__HDR_FILES))

__HDR_GEN_FILES = \
	softwareBuildDate.h
HDR_GEN_FILES = $(addprefix $(SW_SRC_DIR)/, $(__HDR_GEN_FILES))

include $(TARGET_DIR)/$(TARGET).mk

ST7789V_DIR = $(SW_LIBS_DIR)/st7789v
__ST7789V_SRC_FILES = \
	systemFont.c
__ST7789V_HDR_FILES =\
	lv_font.h \
	st7789v.h

ifeq ($(TARGET),bpm_zcu111)
	__ST7789V_SRC_FILES += \
		st7789v.c
else ifeq ($(TARGET),bpm_zcu208)
	__ST7789V_SRC_FILES += \
		st7789v_stub.c
endif

ST7789V_SRC_FILES = $(addprefix $(ST7789V_DIR)/, $(__ST7789V_SRC_FILES))
ST7789V_HDR_FILES = $(addprefix $(ST7789V_DIR)/, $(__ST7789V_HDR_FILES))
SRC_FILES += $(ST7789V_SRC_FILES)
HDR_FILES += $(ST7789V_HDR_FILES)

OBJ_FILES = $(SRC_FILES:.c=.o)
LINKER_FILES = $(SW_SRC_DIR)/lscript.ld

CFLAGS = -Wall -O2 -fmessage-length=0
LD_FLAGS = -Wl,-T -Wl,$(LINKER_FILES)
USER_FLAGS = -DST7789_GRAB_SCREEN -D__BAREMETAL__
LIB_DIRS = $(TARGET)/platform/export/platform/sw/platform/standalone_domain/bsplib/lib
LIB_FLAGS = $(addprefix -L, $(LIB_DIRS))
LIBS = -Wl,--start-group,-lxil,-lgcc,-lc,--end-group -Wl,--start-group,-lxil,-lmetal,-lgcc,-lc,--end-group -Wl,--start-group,-lxil,-llwip4,-lgcc,-lc,--end-group -Wl,--start-group,-lxilffs,-lxil,-lgcc,-lc,--end-group -Wl,--start-group,-lxilpm,-lxil,-lgcc,-lc,--end-group -Wl,--start-group,-lxil,-lgcc,-lc,-lmetal,--end-group

ifeq ($(TARGET),bpm_zcu111)
	USER_FLAGS += -D__TARGET_BPM_ZCU111__
else ifeq ($(TARGET),bpm_zcu208)
	USER_FLAGS += -D__TARGET_BPM_ZCU208__
else
	USER_FLAGS += -D__TARGET_NOT_RECOGNIZED__
endif

INCLUDE_DIRS = \
	$(SW_SRC_DIR) \
	$(TARGET_DIR) \
	$(TARGET)/platform/export/platform/sw/platform/standalone_domain/bspinclude/include \
	$(ST7789V_DIR)
INCLUDE_FLAGS = $(addprefix -I, $(INCLUDE_DIRS))

.PHONY: all clean bsp

vpath *.c $(SW_SRC_DIR) $(ST7789V_DIR) $(TARGET_DIR)
vpath *.h $(SW_SRC_DIR) $(ST7789V_DIR) $(TARGET_DIR)
vpath *.o $(SW_SRC_DIR) $(ST7789V_DIR)
vpath *.bif $(SW_BPM_SCRIPTS_DIR)
vpath *.bif.in $(SW_BPM_SCRIPTS_DIR)

%softwareBuildDate.h: $(SRC_FILES) $(HDR_FILES)
	sh $(SW_BPM_SCRIPTS_DIR)/setSoftwareBuildDate.sh > $@

# FIXME: We are considering BSP done if the directory exists. This
# could lead to mismatches!
$(TARGET):
	$(XSCT) $(SW_SCRIPTS_DIR)/gen_vitis_platform.tcl $(TARGET) $(GW_BPM_TGT_DIR)/system.xsa

$(TARGET).elf: $(HDR_FILES) $(HDR_GEN_FILES) $(OBJ_FILES) $(LINKER_FILES)
	$(CC) $(CFLAGS) $(USER_FLAGS) $(INCLUDE_FLAGS) $(LIB_FLAGS) $(LD_FLAGS) -o $@ $(filter %.o, $^) $(LIBS)

$(TARGET).elf.size: $(TARGET).elf
	$(SIZE) $< | tee $@

# Pull in dependency info for *existing* .o files and don't complain if the
# corresponding .d file is not found
-include $(OBJ_FILES:.o=.d)

%.o: %.c
	$(CC) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" $(CFLAGS) $(USER_FLAGS) $(INCLUDE_FLAGS) -c $*.c -o $@

$(BIN): $(BIF) $(FSBL) $(PMUFW) $(BIT) $(ELF)
	bootgen -image "$<" -w on -arch zynqmp -o "$@"

%.bif: %.bif.in
	sed -e 's|$$BOOTLOADER|$(FSBL)|g' \
		-e 's|$$PMUFW_IMAGE|$(PMUFW)|g' \
		-e 's|$$BIT|$(BIT)|g' \
		-e 's|$$APP|$(ELF)|g' \
		$< > $@

clean::
	$(RM) -rf $(TARGET).elf $(TARGET).elf.size $(HDR_GEN_FILES) \
		$(OBJ_FILES) $(OBJ_FILES:.o=.d) $(TARGET) $(BIF) $(BIN)
