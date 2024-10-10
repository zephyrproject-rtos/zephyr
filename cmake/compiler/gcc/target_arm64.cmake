# SPDX-License-Identifier: Apache-2.0
if(DEFINED GCC_M_CPU)
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})
endif()

if(DEFINED GCC_M_ARCH)
  list(APPEND TOOLCHAIN_C_FLAGS   -march=${GCC_M_ARCH})
  list(APPEND TOOLCHAIN_LD_FLAGS  -march=${GCC_M_ARCH})
endif()

if(DEFINED GCC_M_TUNE)
  list(APPEND TOOLCHAIN_C_FLAGS   -mtune=${GCC_M_TUNE})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mtune=${GCC_M_TUNE})
endif()

list(APPEND TOOLCHAIN_C_FLAGS   -mabi=lp64)
list(APPEND TOOLCHAIN_LD_FLAGS  -mabi=lp64)

set(LLEXT_REMOVE_FLAGS
  -fno-pic
  -fno-pie
  -ffunction-sections
  -fdata-sections
  -g.*
  -Os
)

list(APPEND LLEXT_EDK_REMOVE_FLAGS
  --sysroot=.*
  -fmacro-prefix-map=.*
)

list(APPEND LLEXT_EDK_APPEND_FLAGS
  -nodefaultlibs
)
