# SPDX-License-Identifier: Apache-2.0
list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})

list(APPEND TOOLCHAIN_C_FLAGS   -mabi=lp64)
list(APPEND TOOLCHAIN_LD_FLAGS  -mabi=lp64)
