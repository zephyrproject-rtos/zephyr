# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MAX31790_FAN_SPEED max31790_fan_speed.c)
zephyr_library_sources_ifdef(CONFIG_MAX31790_FAN_FAULT max31790_fan_fault.c)
