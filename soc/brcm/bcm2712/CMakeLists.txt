# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_PINCTRL pinctrl_soc.c)

if(CONFIG_ARM64_GIC_SOC_IRQ_HOOKS)
  zephyr_sources(soc_irq.c)
  zephyr_iterable_section(NAME bcm2712_irq_route GROUP RODATA_REGION)
  zephyr_linker_sources(SECTIONS soc_irq.ld)
endif()

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm64/scripts/linker.ld CACHE INTERNAL "")
