# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  thread.c
  cpu_idle.c
  fatal.c
  irq_manage.c
  swap.S
  prep_c.c
  reset.S
  cache.c
  exception.S
  crt0.S
  )

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
