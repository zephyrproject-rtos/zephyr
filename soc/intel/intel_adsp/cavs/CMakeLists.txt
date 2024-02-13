# Intel ADSP SoCs family CMake file
#
# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)
zephyr_include_directories(include/${SOC_TOOLCHAIN_NAME})
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_library_sources(
    sram.c
    power.c
    power_down_cavs.S
    )

if(CONFIG_SMP OR CONFIG_MP_MAX_NUM_CPUS GREATER 1)
    zephyr_library_sources(multiprocessing.c)
endif()

zephyr_library_sources_ifdef(CONFIG_CAVS_ICTL irq.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/include/xtensa-cavs-linker.ld CACHE INTERNAL "")
