#
# Copyright (c) 2020 Antony Pavlov <antonynpavlov@gmail.com>
#
# based on arch/riscv/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "gccmipsmti")
	if(CONFIG_BIG_ENDIAN)
		set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-tradbigmips")
	else()
		set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-tradlittlemips")
	endif()
else()
	if(CONFIG_BIG_ENDIAN)
		set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-bigmips")
	else()
		set_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT "elf32-littlemips")
	endif()
endif()

add_subdirectory(core)
zephyr_include_directories(include)
