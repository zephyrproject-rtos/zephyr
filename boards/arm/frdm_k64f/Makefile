ifeq ($(BUILD_DTS),1)

always := $(BOARD_NAME).dts_compiled

else

ccflags-y +=-I$(srctree)/drivers

obj-$(CONFIG_PINMUX_MCUX) += pinmux.o

endif
