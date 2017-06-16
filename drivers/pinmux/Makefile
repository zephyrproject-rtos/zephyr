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
obj-$(CONFIG_BOARD_STM32F4_DISCO) += stm32/pinmux_board_stm32f4_disco.o
obj-$(CONFIG_BOARD_STM32F469I_DISCO) += stm32/pinmux_board_stm32f469i_disco.o
obj-$(CONFIG_BOARD_NUCLEO_F411RE) += stm32/pinmux_board_nucleo_f411re.o
obj-$(CONFIG_BOARD_NUCLEO_F412ZG) += stm32/pinmux_board_nucleo_f412zg.o
obj-$(CONFIG_BOARD_NUCLEO_F413ZH) += stm32/pinmux_board_nucleo_f413zh.o
obj-$(CONFIG_BOARD_96B_CARBON) += stm32/pinmux_board_carbon.o
obj-$(CONFIG_BOARD_NUCLEO_L476RG) += stm32/pinmux_board_nucleo_l476rg.o
obj-$(CONFIG_BOARD_NUCLEO_L432KC) += stm32/pinmux_board_nucleo_l432kc.o
obj-$(CONFIG_BOARD_DISCO_L475_IOT1) += stm32/pinmux_board_disco_l475_iot1.o
obj-$(CONFIG_BOARD_STM32L496G_DISCO) += stm32/pinmux_board_stm32l496g_disco.o
obj-$(CONFIG_BOARD_OLIMEXINO_STM32) += stm32/pinmux_board_olimexino_stm32.o
obj-$(CONFIG_BOARD_STM32_MINI_A15) += stm32/pinmux_board_stm32_mini_a15.o
obj-$(CONFIG_PINMUX_QMSI) += pinmux_qmsi.o
obj-$(CONFIG_PINMUX_FE310) += pinmux_fe310.o
obj-$(CONFIG_PINMUX_CC2650) += pinmux_cc2650.o

# "runtime" pinmux
obj-$(CONFIG_PINMUX_DEV) += dev/
