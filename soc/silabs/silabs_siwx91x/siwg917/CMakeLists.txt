# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SIWG917 soc.c)
zephyr_sources_ifdef(CONFIG_SILABS_SIWX91X_NWP siwx91x_nwp.c)
zephyr_sources_ifdef(CONFIG_PM power.c)
zephyr_sources_ifdef(CONFIG_SIWX91X_FIRMWARE_UPGRADE_SHELL siwx91x_fota_shell.c)
zephyr_sources_ifdef(CONFIG_SIWX91X_VERSION_SHELL siwx91x_version_shell.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
