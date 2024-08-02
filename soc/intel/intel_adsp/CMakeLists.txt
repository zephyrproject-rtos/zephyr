# Intel ADSP SoCs family CMake file
#
# Copyright (c) 2020-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(common)

add_subdirectory(common)
if(CONFIG_SOC_SERIES_INTEL_ADSP_ACE)
	zephyr_include_directories(ace)
	add_subdirectory(ace)
endif()
if(CONFIG_INTEL_ADSP_CAVS)
	zephyr_include_directories(cavs)
	add_subdirectory(cavs)
endif()
zephyr_include_directories(common/include)
