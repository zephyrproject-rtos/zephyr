# Copyright (C) 2023, Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
