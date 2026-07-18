# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LLVM_USE_LD)
  set(LINKER ld)
elseif(CONFIG_LLVM_USE_LLD)
  set(LINKER lld)
endif()

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
elseif("${ARCH}" STREQUAL "x86")
  if(CONFIG_64BIT)
    set(triple x86_64-pc-none-elf)
  else()
    set(triple i686-pc-none-elf)
  endif()
elseif("${ARCH}" STREQUAL "riscv")
  if(CONFIG_64BIT)
    set(triple riscv64-unknown-elf)
  else()
    set(triple riscv32-unknown-elf)
  endif()
elseif("${ARCH}" STREQUAL "xtensa")
  # Xtensa uses Clang for compilation with LLD and the Zephyr SDK binutils.
  # The target triple encodes the specific Xtensa core variant.
  #
  # Two names are involved per SoC:
  #   * XTENSA_TOOLCHAIN_TARGET - Zephyr SDK toolchain target dir name
  #                               (matches CONFIG_SOC_TOOLCHAIN_NAME),
  #                               e.g. "intel_ace30_ptl".
  #   * XTENSA_CORE_ID          - upstream LLVM Xtensa backend -mcpu name,
  #                               typically "<vendor>_<id>_adsp",
  #                               e.g. "intel_ace30_adsp".
  #
  # Both can be overridden via environment variables. If unset, they are
  # derived from CONFIG_SOC_TOOLCHAIN_NAME and a small mapping table for
  # the SoCs that ship with mismatched SDK/LLVM names.

  set(XTENSA_TOOLCHAIN_TARGET $ENV{XTENSA_TOOLCHAIN_TARGET})
  if(NOT XTENSA_TOOLCHAIN_TARGET)
    if(CONFIG_SOC_TOOLCHAIN_NAME)
      set(XTENSA_TOOLCHAIN_TARGET "${CONFIG_SOC_TOOLCHAIN_NAME}")
    else()
      # Fallback for boards that don't set CONFIG_SOC_TOOLCHAIN_NAME yet.
      set(XTENSA_TOOLCHAIN_TARGET "intel_ace30_ptl")
    endif()
  endif()

  set(XTENSA_CORE_ID $ENV{XTENSA_CORE_ID})
  if(NOT XTENSA_CORE_ID)
    # Map Zephyr SDK toolchain target -> upstream LLVM Xtensa -mcpu name.
    # Only entries whose names differ between the SDK and LLVM are listed;
    # anything not in this table is assumed to be identical in both.
    set(_xtensa_sdk_to_llvm_cpu
      # Intel ADSP
      intel_ace15_mtpm   intel_ace15_adsp
      intel_ace40        intel_ace40_adsp
      intel_ace30_ptl    intel_ace30_adsp
    )
    list(FIND _xtensa_sdk_to_llvm_cpu "${XTENSA_TOOLCHAIN_TARGET}" _idx)
    if(_idx GREATER -1)
      math(EXPR _val_idx "${_idx} + 1")
      list(GET _xtensa_sdk_to_llvm_cpu ${_val_idx} XTENSA_CORE_ID)
    else()
      # SDK target name matches the LLVM -mcpu name (true for most
      # AMD/MTK/NXP ADSP cores and dc233c / sample_controller*).
      set(XTENSA_CORE_ID "${XTENSA_TOOLCHAIN_TARGET}")
    endif()
    unset(_xtensa_sdk_to_llvm_cpu)
    unset(_idx)
    unset(_val_idx)
  endif()

  set(triple xtensa-${XTENSA_TOOLCHAIN_TARGET}_zephyr-elf)
  set(XTENSA_CLANG_MCPU ${XTENSA_CORE_ID})

  # Use LLVM LLD linker
  set(LINKER lld)
  set(BINTOOLS llvm)

  set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

  # Configure cross-compile prefix for binutils (assembler, linker, objcopy, etc.)
  set(CROSS_COMPILE_TARGET xtensa-${XTENSA_TOOLCHAIN_TARGET}_zephyr-elf)

  # Clang-specific flags for Xtensa
  list(APPEND TOOLCHAIN_C_FLAGS -mcpu=${XTENSA_CLANG_MCPU})
  # Define __XCC__ so SOF's HiFi detection logic (format.h, fft.h, fir_config.h
  # etc.) recognises our LLVM Clang as an Xtensa compiler with HiFi support and
  # enters the "#if defined __XCC__" branches that read XCHAL_HAVE_HIFIx from
  # <xtensa/config/core-isa.h>.
  # Also define __XCC_CLANG__ so the CAVS-family core-isa.h files (tgl, apl,
  # cnl, icl) can whitelist LLVM without triggering their "#error xcc should
  # not use this header" guard (see modules/hal/xtensa patch below).
  list(APPEND TOOLCHAIN_C_FLAGS -D__XCC__ -D__XCC_CLANG__)

  # Use the integrated assembler for all code
  list(APPEND TOOLCHAIN_C_FLAGS -fintegrated-as)

  if(CONFIG_COMPILER_CODEGEN_VLIW_ENABLED)
    # Add +flix strictly to compile steps to prevent ld from misinterpreting +flix as an object file
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX,ASM>:SHELL:-Xclang -target-feature -Xclang +flix>")
  endif()
  # Don't force auto-litpools inline in text sections; place in separate .literal sections instead

  # Force DWARFv4 debug info for compatibility with Zephyr's kobject scanner
  # (gen_kobject_list.py). Clang defaults to DWARFv5 which the scanner can't parse.
  list(APPEND TOOLCHAIN_C_FLAGS -gdwarf-4)

  # FLIX VLIW instruction bundling - DISABLED by default.
  # The packetizer pass in LLVM generates some invalid instruction pairs
  # because we don't fully model the hardware FLIX slot constraints yet.
  # To enable manually: add '-Xclang -target-feature -Xclang +flix' to CMAKE_C_FLAGS
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Xclang -target-feature -Xclang +flix" CACHE STRING "" FORCE)

  # Ensure the dummy compile test can find the Xtensa assembler and linker.
  if(TOOLCHAIN_HOME)
    set(ENV{PATH} "${TOOLCHAIN_HOME}:$ENV{PATH}")
  endif()
  set(_req_flags "")
  if(TOOLCHAIN_HOME)
    list(APPEND _req_flags -B${TOOLCHAIN_HOME})
  endif()
  list(APPEND _req_flags
    -mcpu=${XTENSA_CLANG_MCPU}
    -nostartfiles
    -nostdlib
  )
  string(REPLACE ";" " " _req_flags_str "${_req_flags}")
  set(CMAKE_REQUIRED_FLAGS "${_req_flags_str}")

  if(TOOLCHAIN_HOME)
    list(APPEND TOOLCHAIN_LD_FLAGS -B${TOOLCHAIN_HOME})
    # Derive the per-core SDK ld binary from XTENSA_TOOLCHAIN_TARGET so
    # each Xtensa core uses its matching SDK linker (e.g. tgl, ace15, ace30).
    set(XTENSA_SDK_LD "${TOOLCHAIN_HOME}xtensa-${XTENSA_TOOLCHAIN_TARGET}_zephyr-elf-ld")
  endif()

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostartfiles -nodefaultlibs" CACHE STRING "" FORCE)

  # Override compiler_set_linker_properties to use compiler-rt builtins
  # instead of libgcc; the compiler-rt builtins are distributed with the
  # LLVM toolchain and cover all Xtensa intrinsics used by Zephyr/SOF.
  function(compiler_set_linker_properties)
    set(_clang_rt_dir "${LLVM_TOOLCHAIN_PATH}/lib/clang/23/lib/xtensa-unknown-unknown-elf")
    set_linker_property(PROPERTY lib_include_dir "-L\"${_clang_rt_dir}\"")
    set_linker_property(PROPERTY rt_library "-lclang_rt.builtins")
  endfunction()

  message(STATUS "Xtensa LLVM: triple=${triple} mcpu=${XTENSA_CLANG_MCPU}")

  # .S assembly files need the integrated assembler
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -fintegrated-as -fno-unwind-tables" CACHE STRING "" FORCE)
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

if(DEFINED runtime_lib)
  if("${ARCH}" STREQUAL "xtensa")
    list(APPEND TOOLCHAIN_C_FLAGS --rtlib=${runtime_lib})
  else()
    list(APPEND TOOLCHAIN_C_FLAGS --config=${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
    list(APPEND TOOLCHAIN_LD_FLAGS --config=${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
  endif()
endif()
