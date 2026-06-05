# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BFLB_FREERTOS_SHIM freertos_shim.c)
zephyr_sources_ifdef(CONFIG_BFLB_BTBLE_PORT_OS_SHIM btblecontroller_port_os.c)
add_subdirectory_ifdef(CONFIG_WIFI_BFLB wifi)

# BL61x IEEE 802.15.4 uses the shared wl_api blobs, which need these GLB reset /
# memcpy shims (BL702/BL702L provide their own platform shim instead).
if(CONFIG_IEEE802154_BFLB AND CONFIG_SOC_SERIES_BL61X)
  zephyr_sources(bflb_lmac154_shim.c)
endif()

if(CONFIG_IEEE802154_BFLB AND CONFIG_BUILD_ONLY_NO_BLOBS)
  zephyr_sources(lmac154_stubs.c)
endif()
