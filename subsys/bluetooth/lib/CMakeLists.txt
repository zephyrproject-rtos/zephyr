# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BT_EAD ead.c)
zephyr_sources_ifdef(CONFIG_LIBSBC sbc.c)
