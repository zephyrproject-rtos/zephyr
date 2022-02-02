# Copyright (c) 2021 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_include_directories(.)

include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.psoc6_01.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.psoc6_02.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.psoc6_03.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.psoc6_04.txt)

# PSoC 6 family defines
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_CAT1A COMPONENT_CAT1A)
zephyr_compile_definitions_ifdef(CONFIG_SOC_FAMILY_CAT1 COMPONENT_CAT1)

# Add sections
zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_CAT1 NOINIT noinit.ld)

# Add section for cm0p image ROM
zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_CAT1A ROM_START SORT_KEY 0 rom_cm0image.ld)

# Add section for cm0p image RAM
zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_CAT1A RAM_SECTIONS SORT_KEY 0 ram_cm0image.ld)