# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  vector_table.S
  reset.S
  fault_s.S
  fault.c
  exc_exit.S
  scb.c
  irq_init.c
  thread_abort.c
  )

zephyr_library_sources_ifdef(CONFIG_DEBUG_COREDUMP coredump.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE __aeabi_read_tp.S)

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
