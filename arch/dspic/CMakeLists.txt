# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BIG_ENDIAN)
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-big)
else()
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-little)
endif()
zephyr_include_directories(${ZEPHYR_MICROCHIP_DFP_MODULE_DIR}/support/generic/h/)
zephyr_include_directories(include)
add_subdirectory(core)
