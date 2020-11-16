# SPDX-License-Identifier: Apache-2.0
if(DEFINED GCC_M_CPU)
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})
endif()

if(DEFINED GCC_M_ARCH)
  list(APPEND TOOLCHAIN_C_FLAGS   -march=${GCC_M_ARCH})
  list(APPEND TOOLCHAIN_LD_FLAGS  -march=${GCC_M_ARCH})
endif()

list(APPEND TOOLCHAIN_C_FLAGS   -mabi=lp64)
list(APPEND TOOLCHAIN_LD_FLAGS  -mabi=lp64)
