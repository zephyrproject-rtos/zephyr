# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)

set_target_properties(${ZEPHYR_TARGET} PROPERTIES PROPERTY_OUTPUT_FORMAT elf32-littleriscv)
