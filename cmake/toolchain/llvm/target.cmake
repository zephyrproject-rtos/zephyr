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
  # Xtensa uses Clang for compilation with GCC assembler/linker from Zephyr SDK.
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

  # Use GCC assembler/linker from Zephyr SDK
  set(LINKER ld)
  set(BINTOOLS gnu)

  # Configure cross-compile prefix for binutils (assembler, linker, objcopy, etc.)
  set(CROSS_COMPILE_TARGET xtensa-${XTENSA_TOOLCHAIN_TARGET}_zephyr-elf)
  if(DEFINED ZEPHYR_SDK_INSTALL_DIR)
    set(CROSS_COMPILE $ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-)
  endif()

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
  # Use the GCC assembler for all code — the integrated assembler can't
  # encode HiFi pseudo instructions (AE_MOVDA32X2 etc).
  list(APPEND TOOLCHAIN_C_FLAGS -fno-integrated-as)
  if(CONFIG_COMPILER_CODEGEN_VLIW_ENABLED)
    # Add +flix strictly to compile steps to prevent ld from misinterpreting +flix as an object file
    add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX,ASM>:SHELL:-Xclang -target-feature -Xclang +flix>")
  endif()
  # Place Xtensa literal pools inline with their parent text sections.
  # Without this, the GNU assembler places all literals in a single .literal
  # section, which is unreachable from .imr functions in a different MEMORY region.
  # Also enable --longcalls so the assembler converts call4/call8/call12 into
  # l32r+callxN when the target may be out of the ±512KB direct call range.
  # This fixes cross-section calls (e.g. .text → .imr) that the linker
  # cannot relax without R_XTENSA_ASM_EXPAND relocations.
  list(APPEND TOOLCHAIN_C_FLAGS -Wa,--text-section-literals -Wa,--longcalls)
  # Force DWARFv4 debug info for compatibility with Zephyr's kobject scanner
  # (gen_kobject_list.py). Clang defaults to DWARFv5 which the scanner can't parse.
  list(APPEND TOOLCHAIN_C_FLAGS -gdwarf-4)
  # -B paths: first for assembler (as), second for GCC linker driver
  list(APPEND TOOLCHAIN_C_FLAGS -B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/${CROSS_COMPILE_TARGET}/bin)
  list(APPEND TOOLCHAIN_C_FLAGS -B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin)
  # Add the GCC SDK's xtensa include path for core-isa.h and related HAL headers
  # needed by HiFi code (format_hifi3.h etc.)
  file(GLOB GCC_XTENSA_INCLUDE "$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/lib/gcc/${CROSS_COMPILE_TARGET}/*/include")
  list(APPEND TOOLCHAIN_C_FLAGS -isystem${GCC_XTENSA_INCLUDE})
  list(APPEND TOOLCHAIN_C_FLAGS -isystem$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/${CROSS_COMPILE_TARGET}/include)



  set(SYSROOT_DIR $ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/${CROSS_COMPILE_TARGET})

  # Pre-set the GNU linker so FindGnuLd doesn't fall back to the host x86 linker
  set(GNULD_LINKER $ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-ld.bfd
      CACHE FILEPATH "GNU ld linker" FORCE)
  set(GNULD_LINKER_IS_BFD ON CACHE BOOL "Linker BFD compatibility" FORCE)

  # FLIX VLIW instruction bundling — DISABLED by default.
  # The packetizer pass in LLVM generates some invalid instruction pairs
  # because we don't fully model the hardware FLIX slot constraints yet.
  # To enable manually: add '-Xclang -target-feature -Xclang +flix' to CMAKE_C_FLAGS
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Xclang -target-feature -Xclang +flix" CACHE STRING "" FORCE)

  # Ensure the dummy compile test can find the Xtensa assembler and linker.
  # NOTE: The Xtensa GCC bin dir must be on PATH for Clang's linker-via-gcc to work.
  set(CMAKE_REQUIRED_FLAGS "-B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/${CROSS_COMPILE_TARGET}/bin -B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin -mcpu=${XTENSA_CLANG_MCPU} -nostartfiles -nostdlib")

  # Force cmake to use the Xtensa linker for all link steps.
  # TOOLCHAIN_LD_FLAGS applies to the main zephyr build (via zephyr_ld_options).
  # CMAKE_LINKER is needed for sub-targets like LLEXT.
  # Add -B paths so clang can find the Xtensa linker/assembler when it drives linking.
  set(XTENSA_LD_BFD $ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-ld.bfd)
  set(CMAKE_LINKER ${XTENSA_LD_BFD} CACHE FILEPATH "" FORCE)
  list(APPEND TOOLCHAIN_LD_FLAGS
    -B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/${CROSS_COMPILE_TARGET}/bin
    -B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin
  )
  # Generate a custom GCC specs file that removes picolibc.ld auto-inclusion.
  # The Zephyr SDK GCC is built with --with-default-libc=picolibc which adds
  # -Tpicolibc.ld to every link. picolibc.ld defines a 'flash' MEMORY region
  # and AT>flash LMA mappings that conflict with the RAM-only Xtensa memory model.
  set(XTENSA_SPECS_FILE "${CMAKE_BINARY_DIR}/xtensa_nopico.specs")
  set(_GCC_FOR_SPECS "$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-gcc")
  execute_process(
    COMMAND ${_GCC_FOR_SPECS} -dumpspecs
    OUTPUT_VARIABLE _gcc_specs
  )
  string(REPLACE "%:if-exists-then-else(%:find-file(picolibc.ld) -Tpicolibc.ld)" "" _gcc_specs "${_gcc_specs}")
  file(WRITE "${XTENSA_SPECS_FILE}" "${_gcc_specs}")

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/${CROSS_COMPILE_TARGET}/bin -B$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin -nostartfiles -nodefaultlibs -specs=${XTENSA_SPECS_FILE} -Wl,--noinhibit-exec" CACHE STRING "" FORCE)


  # Override compiler_set_linker_properties to use the GCC libgcc path since
  # we compile with clang but link with GCC. Clang's --print-libgcc-file-name
  # returns the clang compiler-rt path which GCC's ld can't find.
  function(compiler_set_linker_properties)
    set(_gcc_libdir "$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/lib/gcc/${CROSS_COMPILE_TARGET}/14.3.0")
    set_linker_property(PROPERTY lib_include_dir "-L\"${_gcc_libdir}\"")
    set_linker_property(PROPERTY rt_library "-lgcc")
  endfunction()

  message(STATUS "Xtensa LLVM: triple=${triple} mcpu=${XTENSA_CLANG_MCPU}")
  message(STATUS "Xtensa LLVM: CROSS_COMPILE=${CROSS_COMPILE}")
  message(STATUS "Xtensa LLVM: GNULD_LINKER=${GNULD_LINKER}")

  # .S assembly files need the GCC assembler for Xtensa-specific instructions
  set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -fno-integrated-as -fno-unwind-tables" CACHE STRING "" FORCE)


  # LLEXT shared libraries are loadable modules, not standalone executables.
  # Prevent the GCC driver from linking crt0.o/crtn.o into them.
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -nostartfiles -nodefaultlibs" CACHE STRING "" FORCE)

  # Override the shared library link rule with our wrapper script that:
  # 1. Uses objcopy to make .rodata sections writable (fixes "dangerous
  #    relocation in read-only section")
  # 2. Links with GCC + -mtext-section-literals (fixes "l32r literal placed
  #    after use" errors from section ordering)
  set(LLEXT_LINK_WRAPPER "${CMAKE_CURRENT_LIST_DIR}/llext-link-wrapper.sh")
  set(XTENSA_CROSS_PREFIX "$ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-")
  set(CMAKE_C_CREATE_SHARED_LIBRARY
   "${LLEXT_LINK_WRAPPER} ${XTENSA_CROSS_PREFIX} <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>" CACHE STRING "" FORCE)
  set(CMAKE_CXX_CREATE_SHARED_LIBRARY
   "${LLEXT_LINK_WRAPPER} ${XTENSA_CROSS_PREFIX} <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>" CACHE STRING "" FORCE)
  set(CMAKE_C_CREATE_SHARED_MODULE
   "${LLEXT_LINK_WRAPPER} ${XTENSA_CROSS_PREFIX} <LINK_FLAGS> <CMAKE_SHARED_MODULE_CREATE_C_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>" CACHE STRING "" FORCE)
  set(CMAKE_CXX_CREATE_SHARED_MODULE
   "${LLEXT_LINK_WRAPPER} ${XTENSA_CROSS_PREFIX} <LINK_FLAGS> <CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>" CACHE STRING "" FORCE)
  # Set XTENSA_GCC for use by toolchain_linker_finalize() in
  # cmake/linker/ld/target.cmake — clang can't drive the Xtensa linker
  # because it falls back to /usr/bin/ld which doesn't understand Xtensa ELF.
  set(XTENSA_GCC $ENV{ZEPHYR_SDK_INSTALL_DIR}/gnu/${CROSS_COMPILE_TARGET}/bin/${CROSS_COMPILE_TARGET}-gcc)

  # Also set CMAKE_C/CXX_LINK_EXECUTABLE early so LLEXT targets (which are
  # created via add_executable before toolchain_linker_finalize runs) inherit
  # the correct linker. toolchain_linker_finalize will override these again
  # for the main firmware with the proper std lib ordering.
  set(CMAKE_C_LINK_EXECUTABLE
    "${XTENSA_GCC} <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" CACHE STRING "" FORCE)
  set(CMAKE_CXX_LINK_EXECUTABLE
    "${XTENSA_GCC} <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>" CACHE STRING "" FORCE)

  # Disable ccache for link steps — ccache can't properly handle our
  # wrapper script (it caches/eats the output).
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "")
endif()

if(DEFINED triple)
  set(CMAKE_C_COMPILER_TARGET   ${triple})
  set(CMAKE_ASM_COMPILER_TARGET ${triple})
  set(CMAKE_CXX_COMPILER_TARGET ${triple})

  unset(triple)
endif()

if(NOT "${ARCH}" STREQUAL "xtensa")
  if(CONFIG_LIBGCC_RTLIB)
    set(runtime_lib "libgcc")
  elseif(CONFIG_COMPILER_RT_RTLIB)
    set(runtime_lib "compiler_rt")
  endif()

  if(DEFINED runtime_lib)
    list(APPEND TOOLCHAIN_C_FLAGS --config=${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
    list(APPEND TOOLCHAIN_LD_FLAGS --config=${ZEPHYR_BASE}/cmake/toolchain/llvm/clang_${runtime_lib}.cfg)
  endif()
endif()
