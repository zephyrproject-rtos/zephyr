# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_SI7006 si7006)
add_subdirectory_ifdef(CONFIG_SI7055 si7055)
add_subdirectory_ifdef(CONFIG_SI7060 si7060)
add_subdirectory_ifdef(CONFIG_SI7210 si7210)
# zephyr-keep-sorted-stop
