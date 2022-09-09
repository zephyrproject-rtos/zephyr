# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ISA_ARCV2)
  zephyr_compile_options(-mno-sdata)
endif()
