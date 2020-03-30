# SPDX-License-Identifier: Apache-2.0

# Determines what argument to give to -mfpu= based on the
# KConfig'uration and sets this to GCC_M_FPU

if("${ARCH}" STREQUAL "arm")
  if(CONFIG_CPU_CORTEX_A)
    if(CONFIG_CPU_CORTEX_A53)
      if(CONFIG_CPU_HAS_NEON)
        set(GCC_M_FPU neon-fp-armv8)
      else()
        set(GCC_M_FPU fp-armv8)
      endif()
    endif()
  elseif(CONFIG_CPU_CORTEX_R)
    if(CONFIG_CPU_CORTEX_R4 OR CONFIG_CPU_CORTEX_R5) # VFPv3
      if(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
        set(GCC_M_FPU vfpv3-d16)
      elseif(CONFIG_VFP_FEATURE_SINGLE_PRECISION)
        set(GCC_M_FPU vfpv3xd)
      endif()
      if(CONFIG_VFP_FEATURE_HALF_PRECISION)
        set(GCC_M_FPU ${GCC_M_FPU}-fp16)
      endif()
    endif()
  elseif(CONFIG_CPU_CORTEX_M)
    if(CONFIG_CPU_CORTEX_M4) # FPv4
      set(GCC_M_FPU fpv4-sp-d16)
    elseif(CONFIG_CPU_CORTEX_M7 OR CONFIG_CPU_CORTEX_M33) # FPv5
      if(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
        set(GCC_M_FPU fpv5-d16)
      else()
        set(GCC_M_FPU fpv5-sp-d16)
      endif()
    endif()
  endif()
endif()
