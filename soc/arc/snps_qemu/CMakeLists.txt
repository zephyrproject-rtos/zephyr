# SPDX-License-Identifier: Apache-2.0
zephyr_compile_options(-mcpu=${GCC_M_CPU})

if(CONFIG_ISA_ARCV2)
  zephyr_compile_options(-mno-sdata)
endif()
