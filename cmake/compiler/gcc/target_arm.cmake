# SPDX-License-Identifier: Apache-2.0
if(CONFIG_ARM64)
  list(APPEND TOOLCHAIN_C_FLAGS
    -mcpu=${GCC_M_CPU}
    )
  list(APPEND TOOLCHAIN_LD_FLAGS
    -mcpu=${GCC_M_CPU}
    )
else()
  list(APPEND TOOLCHAIN_C_FLAGS   -mcpu=${GCC_M_CPU})
  list(APPEND TOOLCHAIN_LD_FLAGS  -mcpu=${GCC_M_CPU})

  if(CONFIG_COMPILER_ISA_THUMB2)
    list(APPEND TOOLCHAIN_C_FLAGS   -mthumb)
    list(APPEND TOOLCHAIN_LD_FLAGS  -mthumb)
  endif()

  set(ARCH_FOR_cortex-m0        armv6s-m        )
  set(ARCH_FOR_cortex-m0plus    armv6s-m        )
  set(ARCH_FOR_cortex-m3        armv7-m         )
  set(ARCH_FOR_cortex-m4        armv7e-m        )
  set(ARCH_FOR_cortex-m23       armv8-m.base    )
  set(ARCH_FOR_cortex-m33       armv8-m.main+dsp)
  set(ARCH_FOR_cortex-m33+nodsp armv8-m.main    )
  set(ARCH_FOR_cortex-r4        armv7-r         )
  set(ARCH_FOR_cortex-r5        armv7-r+idiv    )

  if(ARCH_FOR_${GCC_M_CPU})
      set(ARCH_FLAG -march=${ARCH_FOR_${GCC_M_CPU}})
  endif()

  list(APPEND TOOLCHAIN_C_FLAGS -mabi=aapcs ${ARCH_FLAG})
  list(APPEND TOOLCHAIN_LD_FLAGS -mabi=aapcs ${ARCH_FLAG})

  # Defines a mapping from GCC_M_CPU to FPU

  if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
    set(PRECISION_TOKEN)
  else()
    set(PRECISION_TOKEN sp-)
  endif()

  set(FPU_FOR_cortex-m4      fpv4-${PRECISION_TOKEN}d16)
  set(FPU_FOR_cortex-m7      fpv5-${PRECISION_TOKEN}d16)
  set(FPU_FOR_cortex-m33     fpv5-${PRECISION_TOKEN}d16)

  if(CONFIG_FLOAT)
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
