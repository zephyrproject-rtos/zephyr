# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_MS56XX ms56xx)
add_subdirectory_ifdef(CONFIG_MS5837 ms5837)
# zephyr-keep-sorted-stop
