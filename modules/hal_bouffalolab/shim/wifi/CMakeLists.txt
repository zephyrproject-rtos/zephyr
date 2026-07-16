# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_BFLB_WL80211_SHIM wl80211_rtos_port.c wl80211_port.c)
zephyr_sources_ifdef(CONFIG_BUILD_ONLY_NO_BLOBS blob_stubs.c)
