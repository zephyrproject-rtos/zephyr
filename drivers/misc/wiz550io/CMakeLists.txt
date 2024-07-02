# Copyright (c) 2024 Grant Ramsay
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Allow access to private W5500 Ethernet header
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/ethernet)

zephyr_library_sources(wiz550io.c)
