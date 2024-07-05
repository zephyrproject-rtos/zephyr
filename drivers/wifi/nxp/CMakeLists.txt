# Copyright 2022-2024 NXP
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_WIFI_NXP)

zephyr_library_sources(nxp_wifi_drv.c)

zephyr_library_sources_ifdef(CONFIG_NXP_WIFI_SHELL nxp_wifi_shell.c)
zephyr_library_link_libraries_ifdef(CONFIG_WIFI_NM_WPA_SUPPLICANT hostap)

if (CONFIG_BUILD_WITH_TFM)
if (NOT CONFIG_NXP_MONOLITHIC_WIFI)
	set_property(TARGET zephyr_property_target
		APPEND PROPERTY TFM_CMAKE_OPTIONS
		-DUSE_TFM_WIFI_FLASH_REGION=ON
)
endif()

if (NOT ${CONFIG_NXP_WIFI_TFM_MBEDTLS_USER_CONFIG_FILE} STREQUAL "")
	set_property(TARGET zephyr_property_target
		APPEND PROPERTY TFM_CMAKE_OPTIONS
		-DTFM_MBEDCRYPTO_PLATFORM_EXTRA_CONFIG_PATH=${WEST_TOPDIR}/${CONFIG_NXP_WIFI_TFM_MBEDTLS_USER_CONFIG_FILE}
)
endif()
endif()
endif()
