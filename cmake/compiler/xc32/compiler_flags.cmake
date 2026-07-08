# Copyright (c) 2026 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

include(${ZEPHYR_BASE}/cmake/compiler/gcc/compiler_flags.cmake)

# Start from the GCC baseline above and override only XC32-specific behavior.

########################################################
# This section covers flags related to optimization.   #
########################################################

set_compiler_property(PROPERTY optimization_speed -O3)

set_compiler_property(PROPERTY optimization_size_aggressive -Os)

# Linker script flag.
set_compiler_property(PROPERTY linker_script -Wl,-T)

# XC-specific code generation tuning flags.
list(APPEND TOOLCHAIN_C_FLAGS -msmart-io=1)

# GCC defaults may inject -nostdinc/-nostdinc++; XC32 requires its toolchain
# system include paths, so clear those properties here.
set_compiler_property(PROPERTY nostdinc)
set_property(TARGET compiler-cpp PROPERTY nostdincxx "")
