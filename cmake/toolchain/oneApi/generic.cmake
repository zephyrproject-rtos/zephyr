# SPDX-License-Identifier: Apache-2.0

if($ENV{ONEAPI_ROOT})
  set_ifndef(ONEAPI_TOOLCHAIN_PATH "$ENV{ONEAPI_ROOT}")
else()
  set_ifndef(ONEAPI_TOOLCHAIN_PATH "$ENV{ONEAPI_TOOLCHAIN_PATH}")
endif()

if(ONEAPI_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${ONEAPI_TOOLCHAIN_PATH}/compiler/latest/linux/bin/)
  set(ONEAPI_PYTHON_PATH ${ONEAPI_TOOLCHAIN_PATH}/intelpython/latest/bin)
endif()

set(ONEAPI_TOOLCHAIN_PATH ${ONEAPI_TOOLCHAIN_PATH} CACHE PATH "oneApi install directory")

set(COMPILER icx)
set(LINKER lld)
set(BINTOOLS oneApi)

if(CONFIG_64BIT)
  set(triple x86_64-pc-none-elf)
else()
  set(triple i686-pc-none-elf)
endif()

set(CMAKE_C_COMPILER_TARGET   ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

message(STATUS "Found toolchain: host (clang/ld)")
