# SPDX-License-Identifier: Apache-2.0

set(ARM_C_FLAGS)

list(APPEND ARM_C_FLAGS   -mcpu=${GCC_M_CPU})

if(CONFIG_COMPILER_ISA_THUMB2)
  list(APPEND ARM_C_FLAGS   -mthumb)
endif()

list(APPEND ARM_C_FLAGS -mabi=aapcs)

if(CONFIG_FPU)
  list(APPEND ARM_C_FLAGS   -mfpu=${GCC_M_FPU})

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
endif()

if(CONFIG_FP16)
  if    (CONFIG_FP16_IEEE)
    list(APPEND ARM_C_FLAGS   -mfp16-format=ieee)
  elseif(CONFIG_FP16_ALT)
    list(APPEND ARM_C_FLAGS   -mfp16-format=alternative)
  endif()
endif()

if(CONFIG_THREAD_LOCAL_STORAGE)
    list(APPEND ARM_C_FLAGS -mtp=soft)
endif()

list(APPEND TOOLCHAIN_C_FLAGS ${ARM_C_FLAGS})
list(APPEND TOOLCHAIN_LD_FLAGS NO_SPLIT ${ARM_C_FLAGS})

# Flags not supported by llext linker
# (regexps are supported and match whole word)
set(LLEXT_REMOVE_FLAGS
  -fno-pic
  -fno-pie
  -ffunction-sections
  -fdata-sections
  -g.*
  -Os
)

# Flags to be added to llext code compilation
set(LLEXT_APPEND_FLAGS
  -mlong-calls
  -mthumb
)
