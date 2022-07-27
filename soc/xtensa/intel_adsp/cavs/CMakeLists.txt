# Intel ADSP SoCs family CMake file
#
# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_library_sources(
    soc.c
    sram.c
    )

if(CONFIG_SMP OR CONFIG_MP_NUM_CPUS GREATER 1)
    zephyr_library_sources(multiprocessing.c)
endif()
