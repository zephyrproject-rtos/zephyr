# SPDX-License-Identifier: Apache-2.0

zephyr_sources()
zephyr_sources_ifdef(CONFIG_SOC_RISCV_SIFIVE_FREEDOM fe310_clock.c)
zephyr_sources_ifdef(CONFIG_SOC_RISCV_SIFIVE_FU540 fu540_clock.c)
zephyr_sources_ifdef(CONFIG_SOC_RISCV_SIFIVE_FU740 fu740_clock.c)

if(CONFIG_RISCV_PMP AND CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_PMP_REGIONS)
  zephyr_sources(pmp_regions.c)
endif()
