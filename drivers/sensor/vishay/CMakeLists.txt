# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_VCNL36825T vcnl36825t)
add_subdirectory_ifdef(CONFIG_VCNL4040 vcnl4040)
add_subdirectory_ifdef(CONFIG_VEML6031 veml6031)
add_subdirectory_ifdef(CONFIG_VEML7700 veml7700)
# zephyr-keep-sorted-stop
