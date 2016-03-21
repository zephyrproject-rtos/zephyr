ccflags-y +=-I$(srctree)/drivers

# Board initialization
obj-$(CONFIG_PINMUX_K64) += frdm_k64f/pinmux_k64.o frdm_k64f/pinmux_board_frdm_k64f.o
obj-$(CONFIG_PINMUX_STM32) += stm32/pinmux_stm32.o
obj-$(CONFIG_BOARD_NUCLEO_F103RB) += stm32/pinmux_board_nucleo_f103rb.o
obj-$(CONFIG_BOARD_STM32_MINI_A15) += stm32/pinmux_board_stm32_mini_a15.o
obj-$(CONFIG_BOARD_ARDUINO_101) += quark_mcu/pinmux_board_arduino_101.o
obj-$(CONFIG_BOARD_ARDUINO_DUE) += sam3x/pinmux_board_arduino_due.o
obj-$(CONFIG_BOARD_GALILEO) += galileo/pinmux_board_galileo.o galileo/pinmux_galileo.o
obj-$(CONFIG_BOARD_QUARK_D2000_CRB) += quark_mcu/pinmux_board_quark_d2000_crb.o
obj-$(CONFIG_BOARD_QUARK_SE_DEVBOARD) += quark_mcu/pinmux_board_quark_se_dev.o

# "runtime" pinmux
obj-$(CONFIG_PINMUX_DEV) += dev/

