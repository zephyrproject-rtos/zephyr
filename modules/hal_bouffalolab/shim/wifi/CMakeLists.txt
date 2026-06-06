# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BFLB_WL80211_SHIM wl80211_rtos_port.c wl80211_port.c)
zephyr_sources_ifdef(CONFIG_BFLB_BL602_OS_SHIM bl602_os_port.c)
if(CONFIG_BUILD_ONLY_NO_BLOBS)
  zephyr_sources_ifdef(CONFIG_BFLB_WL80211_SHIM wl80211_stubs.c)
  zephyr_sources_ifdef(CONFIG_SOC_SERIES_BL60X bl602_firmware_stubs.c)
endif()
