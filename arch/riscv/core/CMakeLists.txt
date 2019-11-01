# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  cpu_idle.c
  fatal.c
  irq_manage.c
  isr.S
  prep_c.c
  reset.S
  swap.S
  thread.c
)

zephyr_sources_if_kconfig(irq_offload.c)
