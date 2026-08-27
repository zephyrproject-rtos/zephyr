# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_HMC5883L hmc5883l)
add_subdirectory_ifdef(CONFIG_MPR mpr)
add_subdirectory_ifdef(CONFIG_SM351LT sm351lt)
# zephyr-keep-sorted-stop
