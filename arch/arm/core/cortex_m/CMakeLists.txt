# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  exc_exit.c
  fault.c
  fault_s.S
  fpu.c
  reset.S
  scb.c
  thread_abort.c
  vector_table.S
  swap_helper.S
  irq_manage.c
  prep_c.c
  thread.c
  cpu_idle.c
  )

zephyr_library_sources_ifndef(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER irq_init.c)
zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.c)
zephyr_library_sources_ifdef(CONFIG_DEBUG_COREDUMP coredump.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE __aeabi_read_tp.S)
zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
zephyr_library_sources_ifdef(CONFIG_PM_S2RAM pm_s2ram.c pm_s2ram.S)
zephyr_library_sources_ifdef(CONFIG_ARCH_CACHE cache.c)
zephyr_library_sources_ifdef(CONFIG_SW_VECTOR_RELAY irq_relay.S)

if(CONFIG_NULL_POINTER_EXCEPTION_DETECTION_DWT)
  zephyr_library_sources(debug.c)
endif()

if(CONFIG_CORTEX_M_DWT)
	if (CONFIG_TIMING_FUNCTIONS)
		zephyr_library_sources(timing.c)
	endif()
endif()

if (CONFIG_SW_VECTOR_RELAY)
if (CONFIG_CPU_CORTEX_M_HAS_VTOR)
  set(relay_vector_table_sort_key relay_vectors)
else()
# Using 0x0 prefix will result in placing the relay vector table section
# at the beginning of ROM_START (i.e before other sections in ROM_START);
# required for CPUs without VTOR, which need to have the exception vector
# table starting at a fixed address at the beginning of ROM.
  set(relay_vector_table_sort_key 0x0relay_vectors)
endif()

zephyr_linker_sources(
  ROM_START
  SORT_KEY ${relay_vector_table_sort_key}
  relay_vector_table.ld
  )
endif()

if (CONFIG_SW_VECTOR_RELAY OR CONFIG_SW_VECTOR_RELAY_CLIENT)
zephyr_linker_sources(
  RAM_SECTIONS
  vt_pointer_section.ld
  )
endif()

if (CONFIG_CPU_CORTEX_M_HAS_VTOR)
zephyr_linker_sources_ifdef(CONFIG_SRAM_VECTOR_TABLE
  RAM_SECTIONS
  # Maybe need to be changed in order to be placed at the beginning of RAM
  # (conflict with code relocation script)
  SORT_KEY 0
  ram_vector_table.ld
)
endif()
