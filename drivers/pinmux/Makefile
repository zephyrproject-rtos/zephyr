ccflags-y +=-I$(srctree)/drivers

# Board initialization
obj-$(CONFIG_PINMUX_K64) += k64/pinmux.o
obj-$(CONFIG_BOARD_FRDM_K64F) += k64/pinmux_board_frdm_k64f.o
obj-$(CONFIG_BOARD_HEXIWEAR_K64) += k64/pinmux_board_hexiwear.o
obj-$(CONFIG_PINMUX_STM32) += stm32/pinmux_stm32.o
obj-$(CONFIG_PINMUX_BEETLE) += beetle/pinmux_board_v2m_beetle.o
obj-$(CONFIG_BOARD_NUCLEO_F103RB) += stm32/pinmux_board_nucleo_f103rb.o
obj-$(CONFIG_BOARD_NUCLEO_F401RE) += stm32/pinmux_board_nucleo_f401re.o
obj-$(CONFIG_BOARD_96B_CARBON) += stm32/pinmux_board_carbon.o
obj-$(CONFIG_BOARD_OLIMEXINO_STM32) += stm32/pinmux_board_olimexino_stm32.o
obj-$(CONFIG_BOARD_STM32_MINI_A15) += stm32/pinmux_board_stm32_mini_a15.o

# "runtime" pinmux
obj-$(CONFIG_PINMUX_DEV) += dev/
