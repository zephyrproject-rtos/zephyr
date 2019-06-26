# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)

if(CONFIG_64BIT)
set(output_format elf64-littleriscv)
else()
set(output_format elf32-littleriscv)
endif()
set_target_properties(${ZEPHYR_TARGET} PROPERTIES PROPERTY_OUTPUT_FORMAT ${output_format})
