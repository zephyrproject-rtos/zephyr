# SPDX-License-Identifier: Apache-2.0

# Board initialization
zephyr_sources_ifdef(CONFIG_PINMUX_LPC11U6X        pinmux_lpc11u6x.c)
zephyr_sources_ifdef(CONFIG_PINMUX_MCUX            pinmux_mcux.c)
zephyr_sources_ifdef(CONFIG_PINMUX_MCUX_LPC        pinmux_mcux_lpc.c)
zephyr_sources_ifdef(CONFIG_PINMUX_RV32M1          pinmux_rv32m1.c)
zephyr_sources_ifdef(CONFIG_PINMUX_STM32           pinmux_stm32.c)
zephyr_sources_ifdef(CONFIG_PINMUX_XEC             pinmux_mchp_xec.c)
