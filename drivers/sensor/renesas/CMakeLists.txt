# Copyright (c) 2024 Ian Morris
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_HS300X hs300x)
add_subdirectory_ifdef(CONFIG_ISL29035 isl29035)
# zephyr-keep-sorted-stop
