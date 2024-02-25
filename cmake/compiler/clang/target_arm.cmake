# SPDX-License-Identifier: Apache-2.0

set(ARM_C_FLAGS)

list(APPEND ARM_C_FLAGS   -mcpu=${GCC_M_CPU})

if(CONFIG_COMPILER_ISA_THUMB2)
  list(APPEND ARM_C_FLAGS   -mthumb)
endif()

list(APPEND ARM_C_FLAGS -mabi=aapcs)

if(CONFIG_FPU)
  if (CONFIG_CPU_CORTEX_M AND "${GCC_M_FPU}" STREQUAL "auto")
    # The clang compiler doesn't allow specify -mfpu=auto unlike gcc
    # It's specified like that for arch armv8.1m-main cores in gcc-m-fpu.cmake
    list(APPEND ARM_C_FLAGS   -mfpu=fp-armv8-fullfp16-${PRECISION_TOKEN}d16)
  else()
    list(APPEND ARM_C_FLAGS   -mfpu=${GCC_M_FPU})
  endif()

  if(CONFIG_DCLS AND NOT CONFIG_FP_HARDABI)
    # If the processor is equipped with VFP and configured in DCLS topology,
    # the FP "hard" ABI must be used in order to facilitate the FP register
    # initialisation and synchronisation.
    set(FORCE_FP_HARDABI TRUE)
  endif()

  if    (CONFIG_FP_HARDABI OR FORCE_FP_HARDABI)
    list(APPEND ARM_C_FLAGS   -mfloat-abi=hard)
  elseif(CONFIG_FP_SOFTABI)
    list(APPEND ARM_C_FLAGS   -mfloat-abi=softfp)
  endif()
else()
  if (CONFIG_ARMV8_M_MAINLINE)
    list(APPEND ARM_C_FLAGS   -mfpu=none)
  endif() # CONFIG_CORTEX_M AND CONFIG_ARMV8_1_M_MAINLINE
endif() # CONFIG_FPU

if(CONFIG_FP16)
  # Clang only supports IEEE 754-2008 format for __fp16. It's enabled by
  # default, so no need to do anything when CONFIG_FP16_IEEE is selected.
  if(CONFIG_FP16_ALT)
    message(FATAL_ERROR "Clang doesn't support ARM alternative format for FP16")
  endif()
endif()
list(APPEND TOOLCHAIN_C_FLAGS ${ARM_C_FLAGS})
list(APPEND TOOLCHAIN_LD_FLAGS NO_SPLIT ${ARM_C_FLAGS})
