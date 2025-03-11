# Makefile - Microchip PIC32CXSG MCU family
#
# Copyright (c) 2024 Microchip
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)

add_subdirectory(common)
add_subdirectory(${SOC_SERIES})
