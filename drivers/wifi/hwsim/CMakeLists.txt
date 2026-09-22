# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
    wifi_hwsim_medium.c
    wifi_hwsim.c
)
zephyr_library_sources_ifdef(CONFIG_WIFI_NM_WPA_SUPPLICANT wifi_hwsim_supp.c)
zephyr_library_link_libraries_ifdef(CONFIG_WIFI_NM_WPA_SUPPLICANT hostap)
zephyr_library_include_directories(.)
