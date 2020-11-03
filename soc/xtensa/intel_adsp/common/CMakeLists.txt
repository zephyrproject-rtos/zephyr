# Intel CAVS SoC family CMake file
#
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(INTEL_ADSP_COMMON)

zephyr_library_named(intel_adsp_common)
zephyr_library_include_directories(include)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)

set_source_files_properties(adsp.c PROPERTIES COMPILE_FLAGS -std=gnu99)
zephyr_library_sources(adsp.c)

zephyr_library_sources(main_entry.S)
zephyr_library_sources(soc.c)
zephyr_library_sources(soc_mp.c)
zephyr_library_sources(trace_out.c)

zephyr_library_link_libraries(INTEL_ADSP_COMMON)

target_include_directories(INTEL_ADSP_COMMON INTERFACE include)
target_link_libraries(INTEL_ADSP_COMMON INTERFACE intel_adsp_common)

# Common CAVS code
if(CONFIG_SOC_SERIES_INTEL_CAVS_V15 OR
   CONFIG_SOC_SERIES_INTEL_CAVS_V18 OR
   CONFIG_SOC_SERIES_INTEL_CAVS_V20 OR
   CONFIG_SOC_SERIES_INTEL_CAVS_V25)
	include(bootloader.cmake)
endif()
