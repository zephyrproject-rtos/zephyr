# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_SCD4X scd4x)
add_subdirectory_ifdef(CONFIG_SGP40 sgp40)
add_subdirectory_ifdef(CONFIG_SHT3XD sht3xd)
add_subdirectory_ifdef(CONFIG_SHT4X sht4x)
add_subdirectory_ifdef(CONFIG_SHTCX shtcx)
add_subdirectory_ifdef(CONFIG_STS4X sts4x)
# zephyr-keep-sorted-stop
