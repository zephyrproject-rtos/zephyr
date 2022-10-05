# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  thread.c
  thread_entry_wrapper.S
  cpu_idle.S
  fatal.c
  fault.c
  fault_s.S
  irq_manage.c
  timestamp.c
  isr_wrapper.S
  regular_irq.S
  switch.S
  prep_c.c
  reset.S
  vector_table.c
  )

zephyr_library_sources_ifdef(CONFIG_ARCH_CACHE cache.c)
zephyr_library_sources_ifdef(CONFIG_ARC_FIRQ fast_irq.S)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)

zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)
zephyr_library_sources_ifdef(CONFIG_ARC_CONNECT arc_connect.c)
zephyr_library_sources_ifdef(CONFIG_ARC_CONNECT arc_smp.c)

zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)

add_subdirectory_ifdef(CONFIG_ARC_CORE_MPU mpu)
add_subdirectory_ifdef(CONFIG_ARC_SECURE_FIRMWARE secureshield)

zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors vector_table.ld)
