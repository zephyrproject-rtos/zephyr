# Intel ADSP SoCs family CMake file
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
if(CONFIG_SOC_SERIES_INTEL_ACE)
	add_subdirectory(ace)
endif()
if(CONFIG_INTEL_ADSP_CAVS)
	add_subdirectory(cavs)
endif()
