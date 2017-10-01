zephyr_cc_option_ifdef(CONFIG_LTO -flto)

set(ARCH_FOR_cortex-m0     armv6-m     )
set(ARCH_FOR_cortex-m0plus armv6-m     )
set(ARCH_FOR_cortex-m3     armv7-m     )
set(ARCH_FOR_cortex-m4     armv7e-m    )
set(ARCH_FOR_cortex-m23    armv8-m.base)
set(ARCH_FOR_cortex-m33    armv8-m.main)

if(${ARCH_FOR_${GCC_M_CPU}})
    set(ARCH_FLAG -march=${ARCH_FOR_${GCC_M_CPU}})
endif()

zephyr_compile_options(
  -mabi=aapcs
  ${TOOLCHAIN_C_FLAGS}
  ${ARCH_FLAG}
  )

add_subdirectory(soc)
add_subdirectory(core)
