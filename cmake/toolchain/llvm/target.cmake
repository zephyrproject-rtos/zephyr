# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LLVM_USE_LD)
  set(LINKER ld)
elseif(CONFIG_LLVM_USE_LLD)
  set(LINKER lld)
endif()

include(${ZEPHYR_BASE}/cmake/compiler/clang/flag-target.cmake)

clang_target_flag("${ARCH}" triple)

if("${ARCH}" STREQUAL "arm")
  set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")
endif()

if(DEFINED triple)
  set(CMAKE_C_COMPILER_TARGET   ${triple})
  set(CMAKE_ASM_COMPILER_TARGET ${triple})
  set(CMAKE_CXX_COMPILER_TARGET ${triple})

  unset(triple)
endif()

if(CONFIG_LIBGCC_RTLIB)
  set(runtime_lib "libgcc")
elseif(CONFIG_COMPILER_RT_RTLIB)
  set(runtime_lib "compiler_rt")
endif()

list(APPEND TOOLCHAIN_C_FLAGS --config
	${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
list(APPEND TOOLCHAIN_LD_FLAGS --config
	${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
