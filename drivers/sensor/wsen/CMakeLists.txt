# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_ITDS wsen_itds)
add_subdirectory_ifdef(CONFIG_WSEN_HIDS wsen_hids)
add_subdirectory_ifdef(CONFIG_WSEN_PADS wsen_pads)
add_subdirectory_ifdef(CONFIG_WSEN_PDUS wsen_pdus)
add_subdirectory_ifdef(CONFIG_WSEN_TIDS wsen_tids)
# zephyr-keep-sorted-stop
