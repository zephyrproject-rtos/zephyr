#
# Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
#
# based on arch/riscv/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BIG_ENDIAN)
	set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-bigmips")
else()
	set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-littlemips")
endif()

add_subdirectory(core)
zephyr_include_directories(include)
