# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BIG_ENDIAN)
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-bigarm)
else()
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-littlearm)
endif()

if(CONFIG_CPU_CORTEX_M0 OR CONFIG_CPU_CORTEX_M0PLUS)
  zephyr_link_libraries_ifdef(CONFIG_CPP atomic)
endif()

add_subdirectory(core)
