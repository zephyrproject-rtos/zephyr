ccflags-y +=-I$(srctree)/drivers

ccflags-$(CONFIG_PINMUX_QMSI) += -I$(CONFIG_QMSI_INSTALL_PATH)/include

# Board initialization
obj-$(CONFIG_PINMUX_MCUX) += pinmux_mcux.o
obj-$(CONFIG_PINMUX_STM32) += stm32/pinmux_stm32.o
obj-$(CONFIG_PINMUX_BEETLE) += beetle/pinmux_board_v2m_beetle.o
obj-$(CONFIG_BOARD_NUCLEO_F103RB) += stm32/pinmux_board_nucleo_f103rb.o
obj-$(CONFIG_BOARD_STM3210C_EVAL) += stm32/pinmux_board_stm3210c_eval.o
obj-$(CONFIG_BOARD_NUCLEO_F334R8) += stm32/pinmux_board_nucleo_f334r8.o
obj-$(CONFIG_BOARD_STM32373C_EVAL) += stm32/pinmux_board_stm32373c_eval.o
obj-$(CONFIG_BOARD_NUCLEO_F401RE) += stm32/pinmux_board_nucleo_f401re.o
obj-$(CONFIG_BOARD_NUCLEO_F411RE) += stm32/pinmux_board_nucleo_f411re.o
obj-$(CONFIG_BOARD_96B_CARBON) += stm32/pinmux_board_carbon.o
obj-$(CONFIG_BOARD_NUCLEO_L476RG) += stm32/pinmux_board_nucleo_l476rg.o
obj-$(CONFIG_BOARD_OLIMEXINO_STM32) += stm32/pinmux_board_olimexino_stm32.o
obj-$(CONFIG_BOARD_STM32_MINI_A15) += stm32/pinmux_board_stm32_mini_a15.o
obj-$(CONFIG_PINMUX_QMSI) += pinmux_qmsi.o

# "runtime" pinmux
obj-$(CONFIG_PINMUX_DEV) += dev/
