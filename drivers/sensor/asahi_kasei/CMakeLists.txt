# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_AK8975 ak8975)
add_subdirectory_ifdef(CONFIG_AKM09918C akm09918c)
# zephyr-keep-sorted-stop
