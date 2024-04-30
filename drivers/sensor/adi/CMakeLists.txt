# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ADLTC2990 adltc2990)
add_subdirectory_ifdef(CONFIG_ADT7310 adt7310)
add_subdirectory_ifdef(CONFIG_ADT7420 adt7420)
add_subdirectory_ifdef(CONFIG_ADXL345 adxl345)
add_subdirectory_ifdef(CONFIG_ADXL362 adxl362)
add_subdirectory_ifdef(CONFIG_ADXL367 adxl367)
add_subdirectory_ifdef(CONFIG_ADXL372 adxl372)
# zephyr-keep-sorted-stop
