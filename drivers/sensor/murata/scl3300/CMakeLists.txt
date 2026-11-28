# Copyright (c) 2026 Sacra Systems Private Limited
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
 scl3300.c
 scl3300_spi.c
 scl3300_transport.c
)
