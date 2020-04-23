# SPDX-License-Identifier: Apache-2.0
if(CONFIG_ARM64)
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})

  list(APPEND TOOLCHAIN_C_FLAGS   -mabi=lp64)
  list(APPEND TOOLCHAIN_LD_FLAGS  -mabi=lp64)
else()
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})

  if(CONFIG_COMPILER_ISA_THUMB2)
    list(APPEND TOOLCHAIN_C_FLAGS   -mthumb)
    list(APPEND TOOLCHAIN_LD_FLAGS  -mthumb)
  endif()

  list(APPEND TOOLCHAIN_C_FLAGS -mabi=aapcs)
  list(APPEND TOOLCHAIN_LD_FLAGS -mabi=aapcs)

  # Defines a mapping from GCC_M_CPU to FPU

  if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    set(PRECISION_TOKEN)
  else()
    set(PRECISION_TOKEN sp-)
  endif()

  set(FPU_FOR_cortex-m4      fpv4-${PRECISION_TOKEN}d16)
  set(FPU_FOR_cortex-m7      fpv5-${PRECISION_TOKEN}d16)
  set(FPU_FOR_cortex-m33     fpv5-${PRECISION_TOKEN}d16)

  if(CONFIG_FPU)
    list(APPEND TOOLCHAIN_C_FLAGS   -mfpu=${FPU_FOR_${GCC_M_CPU}})
    list(APPEND TOOLCHAIN_LD_FLAGS  -mfpu=${FPU_FOR_${GCC_M_CPU}})
    if    (CONFIG_FP_SOFTABI)
      list(APPEND TOOLCHAIN_C_FLAGS   -mfloat-abi=softfp)
      list(APPEND TOOLCHAIN_LD_FLAGS  -mfloat-abi=softfp)
    elseif(CONFIG_FP_HARDABI)
      list(APPEND TOOLCHAIN_C_FLAGS   -mfloat-abi=hard)
      list(APPEND TOOLCHAIN_LD_FLAGS  -mfloat-abi=hard)
    endif()
  endif()
endif()
