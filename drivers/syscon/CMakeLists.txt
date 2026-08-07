# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SYSCON_GENERIC syscon.c)
zephyr_library_sources_ifdef(CONFIG_SYSCON_SHELL syscon_shell.c)
