# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_DPS310 dps310)
add_subdirectory_ifdef(CONFIG_TLE9104_DIAGNOSTICS tle9104)
add_subdirectory_ifdef(CONFIG_XMC4XXX_TEMP xmc4xxx_temp)
# zephyr-keep-sorted-stop
