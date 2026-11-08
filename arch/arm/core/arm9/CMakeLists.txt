# Copyright (c) 2026 Takayuki Goto
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# ARMv5 predates CMSIS-Core; provide a <cmsis_core.h> shim for the common
# Zephyr ARM code that expects the __DSB()/__WFI()/... intrinsics.
zephyr_include_directories(include)

zephyr_library_sources(
  cache.c
  cpu_idle.c
  exc.S
  fault.c
  irq_init.c
  irq_manage.c
  prep_c.c
  reboot.c
  reset.S
  stacks.c
  switch.S
  thread.c
  vector_relocate.c
  vector_table.S
  )

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_ARMV5_FLAT_MMU mmu.c)
