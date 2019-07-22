# Copyright (c) 2018 ispace, inc
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  entry.S
  prep_c.c
  trap.S
  irq.c
  cpu_idle.c
  sched.S
  thread.c
)

zephyr_library_sources_if_kconfig(irq_offload.c)
