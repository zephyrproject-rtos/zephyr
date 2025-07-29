# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_sources_ifdef(CONFIG_MUX_NXP_INPUTMUX		mux_control_nxp_inputmux.c)
