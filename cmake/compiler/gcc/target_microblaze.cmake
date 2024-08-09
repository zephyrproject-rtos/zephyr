# Copyright (c) 2023 Advanced Micro Devices, Inc. (AMD)
# Copyright (c) 2023 Alp Sayin <alpsayin@gmail.com>
# SPDX-License-Identifier: Apache-2.0



list(APPEND TOOLCHAIN_C_FLAGS -mcpu=${CONFIG_CPU_VERSION})

if(DEFINED CONFIG_MICROBLAZE_DATA_IS_TEXT_RELATIVE)
	list(APPEND TOOLCHAIN_C_FLAGS -mpic-data-is-text-relative)
endif()

if(DEFINED CONFIG_MICROBLAZE_USE_BARREL_SHIFT_INSTR)
	list(APPEND TOOLCHAIN_C_FLAGS -mxl-barrel-shift)
endif()

if(DEFINED CONFIG_MICROBLAZE_USE_MUL_INSTR)
	list(APPEND TOOLCHAIN_C_FLAGS -mno-xl-soft-mul)
else()
	list(APPEND TOOLCHAIN_C_FLAGS -mxl-soft-mul)
endif()

if(NOT DEFINED CONFIG_MICROBLAZE_USE_PATTERN_COMPARE_INSTR)
	list(APPEND TOOLCHAIN_C_FLAGS -mxl-pattern-compare)
endif()

if(DEFINED CONFIG_MICROBLAZE_USE_MULHI_INSTR)
	list(APPEND TOOLCHAIN_C_FLAGS -mxl-multiply-high)
endif()

if(DEFINED CONFIG_MICROBLAZE_USE_DIV_INSTR)
	list(APPEND TOOLCHAIN_C_FLAGS -mno-xl-soft-div)
else()
	list(APPEND TOOLCHAIN_C_FLAGS -mxl-soft-div)
endif()

if(DEFINED CONFIG_MICROBLAZE_USE_HARDWARE_FLOAT_INSTR)
	list(APPEND TOOLCHAIN_C_FLAGS -mhard-float)
else()
	list(APPEND TOOLCHAIN_C_FLAGS -msoft-float)
endif()


# Common options
list(APPEND TOOLCHAIN_C_FLAGS -fdollars-in-identifiers)
# TODO: Remove this when gcc microblaze variant oddity is fixed
list(APPEND TOOLCHAIN_C_FLAGS -mlittle-endian)
list(APPEND TOOLCHAIN_LD_FLAGS -mlittle-endian)

# string(REPLACE ";"  "\n " str "${TOOLCHAIN_C_FLAGS}")
# message(STATUS "Final set of C Flags: ${str}")
