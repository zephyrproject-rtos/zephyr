# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_AD2S1210 ad2s1210)
add_subdirectory_ifdef(CONFIG_ADLTC2990 adltc2990)
add_subdirectory_ifdef(CONFIG_ADT7310 adt7310)
add_subdirectory_ifdef(CONFIG_ADT7420 adt7420)
add_subdirectory_ifdef(CONFIG_ADXL345 adxl345)
add_subdirectory_ifdef(CONFIG_ADXL362 adxl362)
add_subdirectory_ifdef(CONFIG_ADXL367 adxl367)
add_subdirectory_ifdef(CONFIG_ADXL372 adxl372)
add_subdirectory_ifdef(CONFIG_SENSOR_MAX32664C max32664c)
# zephyr-keep-sorted-stop
