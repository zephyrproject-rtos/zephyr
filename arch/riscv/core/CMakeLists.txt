# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  fatal.c
  fault.c
  irq_manage.c
  isr.S
  prep_c.c
  reset.S
  swap.S
  thread.c
  userspace.S
)

zephyr_library_sources_if_kconfig(irq_offload.c)
