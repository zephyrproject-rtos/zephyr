# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CFB cfb.c)
zephyr_sources_ifdef(CONFIG_CFB_USE_DEFAULT_FONTS cfb_fonts.c)
zephyr_sources_ifdef(CONFIG_CFB_SHELL cfb_shell.c)

zephyr_linker_sources_ifdef(CONFIG_CFB DATA_SECTIONS check_cfb_fonts.ld)
