#
# Copyright (c) 2019 Antony Pavlov <antonynpavlov@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
