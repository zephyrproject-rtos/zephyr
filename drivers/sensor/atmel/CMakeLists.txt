# Copyright (c) 2025 Atmel Corporation
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory(m90e3x)
# zephyr-keep-sorted-stop

add_subdirectory_ifdef(CONFIG_M90E26 m90e26)
