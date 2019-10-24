# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  vector_table.S
  reset.S
  fault.c
  scb.c
  irq_init.c
  thread_abort.c
  )

zephyr_linker_sources_ifdef(CONFIG_SW_VECTOR_RELAY
  RAM_SECTIONS
  vt_pointer_section.ld
  )

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/arm/include
)
