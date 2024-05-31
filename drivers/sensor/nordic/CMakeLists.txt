# Copyright (c) 2024 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# zephyr-keep-sorted-start
add_subdirectory(temp)
add_subdirectory_ifdef(CONFIG_NPM1300_CHARGER npm1300_charger)
add_subdirectory_ifdef(CONFIG_QDEC_NRFX qdec_nrfx)
# zephyr-keep-sorted-stop
