# SPDX-License-Identifier: Apache-2.0

set_target_properties(${ZEPHYR_TARGET} PROPERTIES PROPERTY_OUTPUT_FORMAT elf32-xtensa-le)
add_subdirectory(core)
