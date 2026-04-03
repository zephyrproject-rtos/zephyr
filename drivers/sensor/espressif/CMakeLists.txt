# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ESP32_TEMP esp32_temp)
add_subdirectory_ifdef(CONFIG_PCNT_ESP32 pcnt_esp32)
# zephyr-keep-sorted-stop
