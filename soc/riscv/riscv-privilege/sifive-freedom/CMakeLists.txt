# SPDX-License-Identifier: Apache-2.0

zephyr_sources()
zephyr_sources_ifdef(CONFIG_SOC_RISCV_SIFIVE_FREEDOM fe310_clock.c)
zephyr_sources_ifdef(CONFIG_SOC_RISCV_SIFIVE_FU540 fu540_clock.c)
zephyr_sources_ifdef(CONFIG_SOC_RISCV_SIFIVE_FU740 fu740_clock.c)
