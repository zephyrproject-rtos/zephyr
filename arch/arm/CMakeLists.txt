zephyr_cc_option_ifdef(CONFIG_LTO -flto)

set(ARCH_FOR_cortex-m0     armv6-m     )
set(ARCH_FOR_cortex-m0plus armv6-m     )
set(ARCH_FOR_cortex-m3     armv7-m     )
set(ARCH_FOR_cortex-m4     armv7e-m    )
set(ARCH_FOR_cortex-m7     armv7e-m    )
set(ARCH_FOR_cortex-m23    armv8-m.base)
set(ARCH_FOR_cortex-m33    armv8-m.main)
set(FPU_FOR_cortex-m4      fpv4-sp-d16)
set(FPU_FOR_cortex-m7      fpv5-d16)
set(FPU_FOR_cortex-m33     fpv5-d16)

if(CONFIG_FLOAT)
  zephyr_cc_option(-mfpu=${FPU_FOR_${GCC_M_CPU}})
  if    (CONFIG_FP_SOFTABI)
    zephyr_cc_option(-mfloat-abi=soft)
  elseif(CONFIG_FP_HARDABI)
    zephyr_cc_option(-mfloat-abi=hard)
  else()
    message(FATAL_ERROR "Expected CONFIG_FP_(SOFT|HARD)ABI to be set")
  endif()
endif()

zephyr_cc_option(
  -ffunction-sections
  -fdata-sections
  )
zephyr_compile_options(
  -mabi=aapcs
  -mthumb
  -mcpu=${GCC_M_CPU}
  -march=${ARCH_FOR_${GCC_M_CPU}}
  )

add_subdirectory(soc/${SOC_PATH})
add_subdirectory(core)
