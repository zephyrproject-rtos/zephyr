# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(retention.c)
zephyr_library_sources_ifdef(CONFIG_RETENTION_BOOT_MODE bootmode.c)
