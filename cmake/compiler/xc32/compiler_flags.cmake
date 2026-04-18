# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)

# Start from the GCC baseline above and override only XC32-specific behavior.

########################################################
# This section covers flags related to optimization.   #
########################################################

set_compiler_property(PROPERTY optimization_debug -O1)

set_compiler_property(PROPERTY optimization_speed -O3)

set_compiler_property(PROPERTY optimization_size -O2)

set_compiler_property(PROPERTY optimization_size_aggressive -Os)

set_compiler_property(PROPERTY no_optimization -O0)

# Linker script flag.
set_compiler_property(PROPERTY linker_script -Wl,-T)

# Treat implicit-int as an error for C sources.
add_compile_options($<$<COMPILE_LANGUAGE:C>:-Werror=implicit-int>)

if(CONFIG_USERSPACE)
  # Keep DWARF data smaller for userspace builds to avoid excessive debug info.
  list(APPEND TOOLCHAIN_C_FLAGS -fno-debug-types-section)
  list(APPEND TOOLCHAIN_C_FLAGS -fno-var-tracking -fno-var-tracking-assignments)
  list(APPEND TOOLCHAIN_CXX_FLAGS -fno-debug-types-section)
  list(APPEND TOOLCHAIN_CXX_FLAGS -fno-var-tracking -fno-var-tracking-assignments)
endif()

# XC-specific code generation tuning flags.
list(APPEND TOOLCHAIN_C_FLAGS -msmart-io=1)

if(CONFIG_CPU_CORTEX_M AND NOT CONFIG_FPU)
  # Keep compiler and linker on a consistent soft-float ABI when no FPU exists.
  list(APPEND TOOLCHAIN_C_FLAGS -mfloat-abi=soft)
  list(APPEND TOOLCHAIN_CXX_FLAGS -mfloat-abi=soft)
  list(APPEND TOOLCHAIN_LD_FLAGS -mfloat-abi=soft)
endif()

# GCC defaults may inject -nostdinc/-nostdinc++; XC32 requires its toolchain
# system include paths, so clear those properties here.
set_compiler_property(PROPERTY nostdinc)
set_property(TARGET compiler-cpp PROPERTY nostdincxx "")