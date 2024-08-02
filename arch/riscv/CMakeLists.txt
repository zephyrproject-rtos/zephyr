# SPDX-License-Identifier: Apache-2.0

add_subdirectory(core)

zephyr_syscall_header(${ZEPHYR_BASE}/include/zephyr/arch/riscv/error.h)

if(CONFIG_64BIT)
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf64-littleriscv)
else()
set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT elf32-littleriscv)
endif()
