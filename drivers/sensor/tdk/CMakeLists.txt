# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ICM42605 icm42605)
add_subdirectory_ifdef(CONFIG_ICM42688 icm42688)
add_subdirectory_ifdef(CONFIG_ICM42X70 icm42x70)
add_subdirectory_ifdef(CONFIG_ICM45686 icm45686)
add_subdirectory_ifdef(CONFIG_ICP101XX icp101xx)
add_subdirectory_ifdef(CONFIG_ICP201XX icp201xx)
add_subdirectory_ifdef(CONFIG_MPU6050 mpu6050)
add_subdirectory_ifdef(CONFIG_MPU9250 mpu9250)
# zephyr-keep-sorted-stop
