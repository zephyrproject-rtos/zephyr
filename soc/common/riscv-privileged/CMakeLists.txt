# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
  soc_irq.S
  vector.S
  )

zephyr_sources_ifndef(CONFIG_RISCV_CUSTOM_INTERRUPT_CONTROLLER soc_common_irq.c)
