# Defines a mapping from GCC_M_CPU to FPU

set(FPU_FOR_cortex-m4      fpv4-sp-d16)
if(CONFIG_FP_DOUBLE_PRECISION)
set(FPU_FOR_cortex-m7      fpv5-d16)
else()
set(FPU_FOR_cortex-m7      fpv5-sp-d16)
endif()
set(FPU_FOR_cortex-m33     fpv5-sp-d16)
