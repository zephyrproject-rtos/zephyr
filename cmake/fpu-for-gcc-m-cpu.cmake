# Defines a mapping from GCC_M_CPU to FPU

if(CONFIG_CPU_HAS_FPU_DOUBLE_PRECISION)
  set(PRECISION_TOKEN)
else()
  set(PRECISION_TOKEN sp-)
endif()

set(FPU_FOR_cortex-m4      fpv4-${PRECISION_TOKEN}d16)
set(FPU_FOR_cortex-m7      fpv5-${PRECISION_TOKEN}d16)
set(FPU_FOR_cortex-m33     fpv5-${PRECISION_TOKEN}d16)
