# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_AMS_AS5600 ams_as5600)
add_subdirectory_ifdef(CONFIG_AMS_IAQ_CORE ams_iAQcore)
add_subdirectory_ifdef(CONFIG_CCS811 ccs811)
add_subdirectory_ifdef(CONFIG_ENS210 ens210)
add_subdirectory_ifdef(CONFIG_TCS3400 tcs3400)
add_subdirectory_ifdef(CONFIG_TMD2620 tmd2620)
add_subdirectory_ifdef(CONFIG_TSL2540 tsl2540)
add_subdirectory_ifdef(CONFIG_TSL2561 tsl2561)
add_subdirectory_ifdef(CONFIG_TSL2591 tsl2591)
# zephyr-keep-sorted-stop
