# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LLVM_USE_LD)
  set(LINKER ld)
elseif(CONFIG_LLVM_USE_LLD)
  set(LINKER lld)
endif()

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

if(DEFINED triple)
  set(CMAKE_C_COMPILER_TARGET   ${triple})
  set(CMAKE_ASM_COMPILER_TARGET ${triple})
  set(CMAKE_CXX_COMPILER_TARGET ${triple})

  unset(triple)
endif()
