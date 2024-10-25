# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_WIFI_W91
  wifi_w91.c
  wifi_w91_l2.c
  )
