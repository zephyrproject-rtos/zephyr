ccflags-$(CONFIG_PINMUX_DEV) +=-I$(srctree)/drivers
ccflags-$(CONFIG_PINMUX_DEV_FRDM_K64F) +=-I$(srctree)/drivers/pinmux/frdm_k64f/
ccflags-$(CONFIG_PINMUX_DEV_QMSI) += -I$(CONFIG_QMSI_INSTALL_PATH)/include

obj-$(CONFIG_PINMUX_DEV_ATMEL_SAM3X) += pinmux_dev_atmel_sam3x.o
obj-$(CONFIG_PINMUX_DEV_FRDM_K64F) += pinmux_dev_frdm_k64f.o
obj-$(CONFIG_PINMUX_DEV_QMSI) += pinmux_dev_qmsi.o
obj-$(CONFIG_PINMUX_DEV_STM32) += pinmux_dev_stm32.o
