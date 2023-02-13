# Copyright (c) 2018 Foundries.io Ltd
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_OPENISA_RV32M1_RI5CY)
  if (CONFIG_RISCV_GENERIC_TOOLCHAIN)
    zephyr_compile_options(-march=rv32imc_zicsr_zifencei)
  else()
    zephyr_compile_options(-march=rv32imcxpulpv2)
  endif()
elseif(CONFIG_SOC_OPENISA_RV32M1_ZERO_RISCY)
  zephyr_compile_options(-march=rv32imc_zicsr_zifencei)
endif()

zephyr_sources(
  vector.S
  soc_irq.S
  wdog.S
  soc.c
)

zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors vector_table.ld)
