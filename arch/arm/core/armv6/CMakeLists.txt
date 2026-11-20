# Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Exception entry and exit, interrupt handling, context switch and thread
# setup are shared with Cortex-A/R; ARM1176 is ARMv6K, which has everything
# that code needs (srs, rfe, cps, ldrex/strex, TPIDRURO).
set(CORTEX_A_R ${CMAKE_CURRENT_SOURCE_DIR}/../cortex_a_r)
zephyr_library_include_directories(${CORTEX_A_R})

zephyr_library_sources(
  ${CORTEX_A_R}/exc.S
  ${CORTEX_A_R}/irq_init.c
  ${CORTEX_A_R}/switch.S
  ${CORTEX_A_R}/thread.c
  cpu_idle.c
  fault.c
  irq_manage.c
  prep_c.c
  reboot.c
  reset.S
  stacks.c
  vector_table.S
  )

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE ${CORTEX_A_R}/isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD ${CORTEX_A_R}/irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE ${CORTEX_A_R}/__aeabi_read_tp.S)
zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
zephyr_library_sources_ifdef(CONFIG_ARCH_CACHE cache.c)
