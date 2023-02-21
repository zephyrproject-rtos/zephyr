# SPDX-License-Identifier: Apache-2.0

# Board initialization
zephyr_sources_ifdef(CONFIG_PINMUX_XEC             pinmux_mchp_xec.c)
