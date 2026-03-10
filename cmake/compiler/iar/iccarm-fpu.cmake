# Copyright (c) 2025 IAR Systems AB
#
# SPDX-License-Identifier: Apache-2.0

# Determines what argument to give to --fpu= based on the
# KConfiguration and sets this to ICCARM_FPU

if(CONFIG_FPU)

  # 32-bit
  if("${ARCH}" STREQUAL "arm")
    if(CONFIG_CPU_AARCH32_CORTEX_R)
      if(CONFIG_CPU_CORTEX_R4 OR CONFIG_CPU_CORTEX_R5) # VFPv3
        if(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
          set(ICCARM_FPU VFPv3_D16)
        elseif(CONFIG_VFP_FEATURE_SINGLE_PRECISION)
          set(ICCARM_FPU VFPv3-SP)
        endif()
        if(CONFIG_VFP_FEATURE_HALF_PRECISION)
          set(ICCARM_FPU ${ICCARM_FPU}_Fp16)
        endif()
      elseif(CONFIG_CPU_CORTEX_R52)
        if(CONFIG_VFP_FEATURE_DOUBLE_PRECISION)
          set(ICCARM_FPU VFPv5_D16)
        elseif(CONFIG_VFP_FEATURE_SINGLE_PRECISION)
          set(ICCARM_FPU VFPv5-SP)
        endif()
      endif()
    elseif(CONFIG_CPU_CORTEX_M)
      # Defines a mapping from ICCARM_CPU to FPU
      if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
        set(PRECISION_TOKEN _D16)
      else()
        set(PRECISION_TOKEN _SP)
      endif()

      set(FPU_FOR_Cortex-M4           FPv4${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M7           FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M33          FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M33.no_dsp   FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M55          FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M55.no_mve   FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M55.no_dsp   FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M85          FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M85.no_mve   FPv5${PRECISION_TOKEN})
      set(FPU_FOR_Cortex-M85.no_dsp   FPv5${PRECISION_TOKEN})

      set(ICCARM_FPU ${FPU_FOR_${ICCARM_CPU}})
    endif()
  # 64-bit
  else()
    set(ICCARM_FPU none)
  endif()

endif() #CONFIG_FPU
