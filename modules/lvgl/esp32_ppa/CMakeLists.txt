# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  lvgl_ppa.c
  lvgl_ppa_buf.c
  lvgl_ppa_fill.c
)
zephyr_library_sources_ifdef(CONFIG_LV_Z_USE_ESP32_PPA_IMG lvgl_ppa_img.c)
