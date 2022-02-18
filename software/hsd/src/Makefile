all: softwareBuildDate.h
softwareBuildDate.h: acquisition.c \
                     acquisition.h \
					 afe.c \
					 afe.h \
                     config.h \
                     console.c \
                     console.h \
                     display.c \
                     display.h \
                     epics.c \
                     epics.h \
					 epicsApplicationCommands.c \
					 epicsApplicationCommands.h \
                     evr.c \
                     evr.h \
                     eyescan.c \
                     eyescan.h \
                     ffs.c \
                     ffs.h \
                     frequencyMonitor.c \
                     frequencyMonitor.h \
                     gpio.h \
                     hsdProtocol.h \
                     iic.c \
                     iic.h \
                     interlock.c \
                     interlock.h \
                     lmk04208.h \
                     lmx2594.h \
                     lv_font.h \
                     main.c \
                     mgt.c \
                     mgt.h \
                     mmcm.c \
                     mmcm.h \
                     platform.h \
                     platform_config.h \
                     platform_zynqmp.c \
                     rfadc.c \
                     rfadc.h \
                     rfclk.c \
                     rfclk.h \
                     st7789v.c \
                     st7789v.h \
                     sysmon.c \
                     sysmon.h \
                     systemFont.c \
                     systemParameters.c \
                     systemParameters.h \
                     tftp.c \
                     tftp.h \
                     user_mgt_refclk.c \
                     user_mgt_refclk.h \
                     util.c \
                     util.h
	sh setSoftwareBuildDate.sh

lmk04208.h: lmk04208.tcs
	sh ../scripts/createRFCLKheader.sh

lmx2594.h: lmx2594.tcs
	sh ../scripts/createRFCLKheader.sh

# Imported stuff
ST7789V_SOURCE = ../libs/st7789v

lv_font.h: $(ST7789V_SOURCE)/lv_font.h
	cp $< $@
systemFont.c: $(ST7789V_SOURCE)/systemFont.c
	cp $< $@
st7789v.h: $(ST7789V_SOURCE)/st7789v.h
	cp $< $@
st7789v.c: $(ST7789V_SOURCE)/st7789v.c
	cp $< $@
