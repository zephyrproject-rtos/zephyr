# SPDX-License-Identifier: Apache-2.0

if("${ARCH}" STREQUAL "arm")
  if(DEFINED CONFIG_ARMV8_M_MAINLINE)
    # ARMv8-M mainline is ARMv7-M with additional features from ARMv8-M.
    set(triple armv8m.main-none-eabi)
  elseif(DEFINED CONFIG_ARMV8_M_BASELINE)
    # ARMv8-M baseline is ARMv6-M with additional features from ARMv8-M.
    set(triple armv8m.base-none-eabi)
  elseif(DEFINED CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
    # ARMV7_M_ARMV8_M_MAINLINE means that ARMv7-M or backward compatible ARMv8-M
    # processor is used.
    set(triple armv7m-none-eabi)
  elseif(DEFINED CONFIG_ARMV6_M_ARMV8_M_BASELINE)
    # ARMV6_M_ARMV8_M_BASELINE means that ARMv6-M or ARMv8-M supporting the
    # Baseline implementation processor is used.
    set(triple armv6m-none-eabi)
  else()
    # Default ARM target supported by all processors.
    set(triple arm-none-eabi)
  endif()
elseif("${ARCH}" STREQUAL "arm64")
  set(triple aarch64-none-elf)
endif()

if(DEFINED triple)
  set(CMAKE_C_COMPILER_TARGET   ${triple})
  set(CMAKE_ASM_COMPILER_TARGET ${triple})
  set(CMAKE_CXX_COMPILER_TARGET ${triple})

  unset(triple)
endif()

# With clang 20 when option '--rtlib=compiler-rt' is appenned to the build
# command line, it may rise some false positive warning:
# ---> clang: warning: argument unused during compilation: '--rtlib=compiler-rt' [-Wunused-command-line-argument]
# LLVM issue [64939](https://github.com/llvm/llvm-project/issues/64939)
# By passing the argument through --config <config_file> the warning does not show up.

set(runtime_lib "compiler_rt")

list(APPEND TOOLCHAIN_C_FLAGS --config
	${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
list(APPEND TOOLCHAIN_LD_FLAGS --config
	${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
