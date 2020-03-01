# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if (CONFIG_COVERAGE)
  toolchain_cc_coverage()
endif ()

zephyr_library_sources(
  cpu_idle.S
  fatal.c
  irq_manage.c
  prep_c.c
  reset.S
  swap.c
  swap_helper.S
  thread.c
  vector_table.S
)

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_ARM_MMU arm_mmu.c)
