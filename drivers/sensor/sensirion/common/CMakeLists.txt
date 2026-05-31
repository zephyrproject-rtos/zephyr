# Copyright (c) 2025 Sensirion
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(conversions.c)
zephyr_library_sources(i2c_packet.c)
zephyr_library_sources(i2c_general_call_reset.c)
