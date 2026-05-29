# Makefile - Atmel SAM0 MCU family
#
# Copyright (c) 2017 Google LLC.
# Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)

add_subdirectory(common)
add_subdirectory(${SOC_SERIES})
