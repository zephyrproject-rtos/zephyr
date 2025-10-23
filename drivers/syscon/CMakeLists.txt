# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SYSCON_GENERIC          syscon.c)
zephyr_library_sources_ifdef(CONFIG_SYSCON_SHELL            syscon_shell.c)

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_SYSCON_BFLB_EFUSE       syscon_bflb_efuse.c)
# zephyr-keep-sorted-stop
