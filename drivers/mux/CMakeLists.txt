# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_sources_ifdef(CONFIG_MUX_REG_MUX                 mux_reg_mux.c)
