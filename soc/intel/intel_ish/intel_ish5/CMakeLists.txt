# Copyright (c) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)

zephyr_cc_option(-march=pentium -mtune=i486)

zephyr_sources(soc.c)
add_subdirectory_ifdef(CONFIG_PM pm)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")

include(../utils/build_ish_firmware.cmake)
