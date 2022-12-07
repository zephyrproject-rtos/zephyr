# Intel ADSP SoCs family CMake file
#
# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_library_sources(
    sram.c
    power.c
    )

if(CONFIG_SMP OR CONFIG_MP_MAX_NUM_CPUS GREATER 1)
    zephyr_library_sources(multiprocessing.c)
endif()

zephyr_library_sources_ifdef(CONFIG_CAVS_ICTL irq.c)
