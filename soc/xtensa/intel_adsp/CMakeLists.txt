# Intel CAVS SoC family CMake file
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_INTEL_ADSP_BAYTRAIL	${SOC_SERIES})
