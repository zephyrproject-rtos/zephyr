# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_MS5607 ms5607)
add_subdirectory_ifdef(CONFIG_MS5837 ms5837)
# zephyr-keep-sorted-stop
