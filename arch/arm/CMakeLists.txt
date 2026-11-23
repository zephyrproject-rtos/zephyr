# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BIG_ENDIAN)
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-bigarm)
else()
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-littlearm)
endif()

add_subdirectory(core)
