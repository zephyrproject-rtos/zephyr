# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_FATAL fault.c)
zephyr_library_sources_if_kconfig(reset.S)
zephyr_library_sources_ifdef(CONFIG_THREAD thread_abort.c)
zephyr_library_sources_ifdef(CONFIG_ARM_SCB scb.c)
zephyr_library_sources_ifdef(CONFIG_ARM_IRQ_INIT irq_init.c)
zephyr_library_sources_ifdef(CONFIG_ARM_VECTOR_TABLE vector_table.S)
zephyr_linker_sources_ifdef(CONFIG_SW_VECTOR_RELAY
  RAM_SECTIONS
  vt_pointer_section.ld
  )
