# SPDX-License-Identifier: Apache-2.0

# Determines what argument to give to -mfpu= based on the
# KConfig'uration and sets this to GCC_M_FPU

if(CONFIG_FPU)

if("${ARCH}" STREQUAL "arm")
  if(CONFIG_CPU_AARCH32_CORTEX_R)
    if(CONFIG_CPU_CORTEX_R4 OR CONFIG_CPU_CORTEX_R5 OR CONFIG_CPU_CORTEX_R8) # VFPv3
      if(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
        set(GCC_M_FPU vfpv3-d16)
      elseif(CONFIG_VFP_FEATURE_SINGLE_PRECISION)
        set(GCC_M_FPU vfpv3xd)
      endif()
      if(CONFIG_VFP_FEATURE_HALF_PRECISION)
        set(GCC_M_FPU ${GCC_M_FPU}-fp16)
      endif()
    elseif(CONFIG_CPU_CORTEX_R52)
      if(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
        set(GCC_M_FPU neon-fp-armv8)
      elseif(CONFIG_VFP_FEATURE_SINGLE_PRECISION)
        set(GCC_M_FPU fpv5-sp-d16)
      endif()
    endif()
  elseif(CONFIG_CPU_CORTEX_M)
    # Defines a mapping from GCC_M_CPU to FPU
    if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
      set(PRECISION_TOKEN)
    else()
      set(PRECISION_TOKEN sp-)
    endif()

    set(FPU_FOR_cortex-m4           fpv4-${PRECISION_TOKEN}d16)
    set(FPU_FOR_cortex-m7           fpv5-${PRECISION_TOKEN}d16)
    set(FPU_FOR_cortex-m33          fpv5-${PRECISION_TOKEN}d16)
    set(FPU_FOR_cortex-m33+nodsp    fpv5-${PRECISION_TOKEN}d16)
    set(FPU_FOR_cortex-m55          auto)
    set(FPU_FOR_cortex-m55+nomve.fp auto)
    set(FPU_FOR_cortex-m55+nomve    auto)
    set(FPU_FOR_cortex-m55+nodsp    auto)
    set(FPU_FOR_cortex-m85          auto)
    set(FPU_FOR_cortex-m85+nomve.fp auto)
    set(FPU_FOR_cortex-m85+nomve    auto)
    set(FPU_FOR_cortex-m85+nodsp    auto)

    set(GCC_M_FPU ${FPU_FOR_${GCC_M_CPU}})
  elseif(CONFIG_CPU_AARCH32_CORTEX_A)
    if(CONFIG_CPU_CORTEX_A7)
      set(GCC_M_FPU vfpv4-d16)
    endif()
  endif()
endif()

endif() #CONFIG_FPU
