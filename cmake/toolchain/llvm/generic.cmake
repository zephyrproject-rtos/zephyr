# SPDX-License-Identifier: Apache-2.0

# Todo: deprecate CLANG_ROOT_DIR
set_ifndef(LLVM_TOOLCHAIN_PATH "$ENV{CLANG_ROOT_DIR}")
zephyr_get(LLVM_TOOLCHAIN_PATH)

if(LLVM_TOOLCHAIN_PATH)
  set(TOOLCHAIN_HOME ${LLVM_TOOLCHAIN_PATH}/bin/)
endif()

set(LLVM_TOOLCHAIN_PATH ${CLANG_ROOT_DIR} CACHE PATH "clang install directory")

set(COMPILER clang)
if(CONFIG_LLVM_USE_LD)
  set(LINKER ld)
else()
  set(LINKER lld)
endif()
set(BINTOOLS llvm)

if("${ARCH}" STREQUAL "arm")
  set(triple arm-none-eabi)
  set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")
elseif("${ARCH}" STREQUAL "x86")
  if(CONFIG_64BIT)
    set(triple x86_64-pc-none-elf)
  else()
    set(triple i686-pc-none-elf)
  endif()
endif()

set(CMAKE_C_COMPILER_TARGET   ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})

set(TOOLCHAIN_HAS_NEWLIB OFF CACHE BOOL "True if toolchain supports newlib")

message(STATUS "Found toolchain: host (clang/ld)")
