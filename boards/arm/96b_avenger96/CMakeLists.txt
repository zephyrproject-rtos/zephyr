# Copyright (c) 2019 Linaro Ltd.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PINMUX)
zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
endif()
