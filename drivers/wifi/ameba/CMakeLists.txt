# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_AMEBA AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  zephyr_library_sources(ameba_wifi_drv.c)
endif()
