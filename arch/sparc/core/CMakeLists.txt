# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  fatal.c
  reset_trap.S
  prep_c.c
  switch.S
  interrupt_trap.S
  fault_trap.S
  irq_manage.c
  thread.c
  window_trap.S
  sw_trap_set_pil.S
  trap_table_mvt.S
)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
