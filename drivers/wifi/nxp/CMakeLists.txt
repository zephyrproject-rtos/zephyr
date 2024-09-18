# Copyright 2022-2024 NXP
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_WIFI_NXP)

zephyr_library_sources(nxp_wifi_drv.c)

zephyr_library_sources_ifdef(CONFIG_NXP_WIFI_SHELL nxp_wifi_shell.c)
zephyr_library_link_libraries_ifdef(CONFIG_WIFI_NM_WPA_SUPPLICANT hostap)
endif()
