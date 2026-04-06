# Copyright (c) 2026, Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(fuel_gauge_adp5360.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/mfd)
