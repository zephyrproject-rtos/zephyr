ccflags-y +=-I$(srctree)/drivers

ccflags-$(CONFIG_PINMUX_QMSI) += -I$(CONFIG_QMSI_INSTALL_PATH)/include

# Board initialization
obj-$(CONFIG_PINMUX_MCUX) += pinmux_mcux.o
obj-$(CONFIG_PINMUX_STM32) += pinmux_stm32.o
obj-$(CONFIG_PINMUX_BEETLE) += beetle/pinmux_board_v2m_beetle.o
obj-$(CONFIG_PINMUX_QMSI) += pinmux_qmsi.o
obj-$(CONFIG_PINMUX_FE310) += pinmux_fe310.o

# "runtime" pinmux
obj-$(CONFIG_PINMUX_DEV) += dev/
