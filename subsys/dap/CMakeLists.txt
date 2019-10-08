# Copyright (c) 2019, PHYTEC Messtechnik GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()
zephyr_library_sources(cmsis_dap.c)
