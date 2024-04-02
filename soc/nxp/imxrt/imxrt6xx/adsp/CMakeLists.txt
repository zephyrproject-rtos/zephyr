# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

zephyr_compile_definitions(CPU_MIMXRT685SFVKB_dsp)

zephyr_include_directories(include)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
