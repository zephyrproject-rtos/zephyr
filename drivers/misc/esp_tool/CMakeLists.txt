# Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ESP_TOOL esp_tool.c)
zephyr_library_sources_ifdef(CONFIG_ESP_TOOL_SHELL shell.c)

if (CONFIG_ESP_TOOL_FW_ARRAYS)
include(bin2array.cmake)
endif()
