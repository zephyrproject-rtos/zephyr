# SPDX-License-Identifier: Apache-2.0

set(ARCH_FOR_cortex-m0        armv6s-m        )
set(ARCH_FOR_cortex-m0plus    armv6s-m        )
set(ARCH_FOR_cortex-m3        armv7-m         )
set(ARCH_FOR_cortex-m4        armv7e-m        )
set(ARCH_FOR_cortex-m23       armv8-m.base    )
set(ARCH_FOR_cortex-m33       armv8-m.main+dsp)
set(ARCH_FOR_cortex-m33+nodsp armv8-m.main    )
set(ARCH_FOR_cortex-r4        armv7-r         )

if(ARCH_FOR_${GCC_M_CPU})
    set(ARCH_FLAG -march=${ARCH_FOR_${GCC_M_CPU}})
endif()

zephyr_compile_options(
  -mabi=aapcs
  ${ARCH_FLAG}
  )

zephyr_ld_options(
  -mabi=aapcs
  ${ARCH_FLAG}
  )

add_subdirectory(core)
