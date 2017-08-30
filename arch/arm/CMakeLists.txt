zephyr_cc_option_ifdef(CONFIG_LTO -flto)

if    (CONFIG_CPU_CORTEX_M0)
  set(GCC_M_CPU cortex-m0)

elseif(CONFIG_CPU_CORTEX_M0PLUS)
  set(GCC_M_CPU cortex-m0plus)

elseif(CONFIG_CPU_CORTEX_M3)
  set(GCC_M_CPU cortex-m3)

elseif(CONFIG_CPU_CORTEX_M4)
  set(GCC_M_CPU cortex-m4)

elseif(CONFIG_CPU_CORTEX_M7)
  set(GCC_M_CPU cortex-m7)

elseif(CONFIG_CPU_CORTEX_M23)
  set(GCC_M_CPU cortex-m23)

elseif(CONFIG_CPU_CORTEX_M33)
  set(GCC_M_CPU cortex-m33)

else()
  message(FATAL_ERROR "Expected CONFIG_CPU_CORTEX_x to be defined")
endif()

set(ARCH_FOR_cortex-m0     armv6-m      )
set(ARCH_FOR_cortex-m0plus armv6-m      )
set(ARCH_FOR_cortex-m3     armv7-m      )
set(ARCH_FOR_cortex-m4     armv7e-m     )
set(ARCH_FOR_cortex-m7     armv7e-m     )
set(ARCH_FOR_cortex-m23    armv8-m.base )
set(ARCH_FOR_cortex-m33    armv8-m.main )

if(CONFIG_FLOAT)
  set(FPU_FOR_cortex-m4  fpv4-sp-d16)
  set(FPU_FOR_cortex-m7  fpv5-d16)
  set(FPU_FOR_cortex-m33 fpv5-d16)
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
