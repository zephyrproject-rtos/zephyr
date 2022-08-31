# SPDX-License-Identifier: Apache-2.0
list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})

if(CONFIG_COMPILER_ISA_THUMB2)
  list(APPEND TOOLCHAIN_C_FLAGS   -mthumb)
  list(APPEND TOOLCHAIN_LD_FLAGS  -mthumb)
endif()

list(APPEND TOOLCHAIN_C_FLAGS -mabi=aapcs)
list(APPEND TOOLCHAIN_LD_FLAGS -mabi=aapcs)

if(CONFIG_FPU)
  list(APPEND TOOLCHAIN_C_FLAGS   -mfpu=${GCC_M_FPU})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mfpu=${GCC_M_FPU})

  if(CONFIG_DCLS AND NOT CONFIG_FP_HARDABI)
    # If the processor is equipped with VFP and configured in DCLS topology,
    # the FP "hard" ABI must be used in order to facilitate the FP register
    # initialisation and synchronisation.
    set(FORCE_FP_HARDABI TRUE)
  endif()

  if    (CONFIG_FP_HARDABI OR FORCE_FP_HARDABI)
    list(APPEND TOOLCHAIN_C_FLAGS   -mfloat-abi=hard)
    list(APPEND TOOLCHAIN_LD_FLAGS  -mfloat-abi=hard)
  elseif(CONFIG_FP_SOFTABI)
    list(APPEND TOOLCHAIN_C_FLAGS   -mfloat-abi=softfp)
    list(APPEND TOOLCHAIN_LD_FLAGS  -mfloat-abi=softfp)
  endif()
endif()

if(CONFIG_FP16)
  if    (CONFIG_FP16_IEEE)
    list(APPEND TOOLCHAIN_C_FLAGS   -mfp16-format=ieee)
    list(APPEND TOOLCHAIN_LD_FLAGS  -mfp16-format=ieee)
  elseif(CONFIG_FP16_ALT)
    list(APPEND TOOLCHAIN_C_FLAGS   -mfp16-format=alternative)
    list(APPEND TOOLCHAIN_LD_FLAGS  -mfp16-format=alternative)
  endif()
endif()
