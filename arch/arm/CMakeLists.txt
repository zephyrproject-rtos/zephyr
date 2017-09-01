zephyr_cc_option_ifdef(CONFIG_LTO -flto)

set(ARCH_FOR_cortex-m0     armv6-m     )
set(ARCH_FOR_cortex-m0plus armv6-m     )
set(ARCH_FOR_cortex-m3     armv7-m     )
set(ARCH_FOR_cortex-m4     armv7e-m    )
set(ARCH_FOR_cortex-m7     armv7e-m    )
set(ARCH_FOR_cortex-m23    armv8-m.base)
set(ARCH_FOR_cortex-m33    armv8-m.main)

zephyr_cc_option(
  -ffunction-sections
  -fdata-sections
  )
zephyr_compile_options(
  -mabi=aapcs
  ${TOOLCHAIN_C_FLAGS}
  -march=${ARCH_FOR_${GCC_M_CPU}}
  )

add_subdirectory(soc)
add_subdirectory(core)
