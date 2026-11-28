# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)

if(CONFIG_CLOCK_CONTROL_SAMA7G5)
  set(sam_pmc_dir ${ZEPHYR_BASE}/soc/microchip/sam/common)

  zephyr_include_directories(${sam_pmc_dir})
  zephyr_sources(
    clk_pic32cz_ca70.c
    ${sam_pmc_dir}/clk-main.c
    ${sam_pmc_dir}/clk-master-mckr.c
    ${sam_pmc_dir}/clk-peripheral.c
    ${sam_pmc_dir}/clk-plla.c
    ${sam_pmc_dir}/pmc.c
  )
endif()
