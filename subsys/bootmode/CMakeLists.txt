# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_BOOT_MODE_RETENTION bootmode_retention.c)
zephyr_library_sources_ifdef(CONFIG_BOOT_MODE_SHELL bootmode_shell.c)
