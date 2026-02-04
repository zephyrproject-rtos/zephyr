# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_sources_ifdef(CONFIG_MUX_REG_MUX				mux_reg_mux.c)
zephyr_sources_ifdef(CONFIG_MUX_NXP_INPUTMUX			mux_nxp_inputmux.c)
zephyr_sources_ifdef(CONFIG_MUX_GPIO_MUX			mux_gpio_mux.c)
