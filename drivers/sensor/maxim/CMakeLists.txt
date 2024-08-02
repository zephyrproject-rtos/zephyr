# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_DS18B20 ds18b20)
add_subdirectory_ifdef(CONFIG_MAX17055 max17055)
add_subdirectory_ifdef(CONFIG_MAX17262 max17262)
add_subdirectory_ifdef(CONFIG_MAX30101 max30101)
add_subdirectory_ifdef(CONFIG_MAX31790_SENSOR max31790)
add_subdirectory_ifdef(CONFIG_MAX31855 max31855)
add_subdirectory_ifdef(CONFIG_MAX31865 max31865)
add_subdirectory_ifdef(CONFIG_MAX31875 max31875)
add_subdirectory_ifdef(CONFIG_MAX44009 max44009)
add_subdirectory_ifdef(CONFIG_MAX6675 max6675)
# zephyr-keep-sorted-stop
