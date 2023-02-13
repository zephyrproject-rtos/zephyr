# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_PCNT_ESP32 pcnt_esp32.c)
