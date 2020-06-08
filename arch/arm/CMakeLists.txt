# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARM64)
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf64-littleaarch64)

  add_subdirectory(core/aarch64)
else()
  set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-littlearm)

  add_subdirectory(core/aarch32)
endif()
