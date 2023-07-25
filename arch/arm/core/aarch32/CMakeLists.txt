# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.S
  fatal.c
  irq_manage.c
  nmi.c
  nmi_on_reset.S
  prep_c.c
  swap.c
  swap_helper.S
  thread.c
)

zephyr_library_sources_ifdef(CONFIG_GEN_SW_ISR_TABLE isr_wrapper.S)
zephyr_library_sources_ifdef(CONFIG_CPP __aeabi_atexit.c)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_SW_VECTOR_RELAY irq_relay.S)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE ../common/tls.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)
zephyr_library_sources_ifdef(CONFIG_ARM_ZIMAGE_HEADER header.S)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M cortex_m)
add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M_HAS_CMSE cortex_m/cmse)
add_subdirectory_ifdef(CONFIG_ARM_SECURE_FIRMWARE cortex_m/tz)
add_subdirectory_ifdef(CONFIG_ARM_NONSECURE_FIRMWARE cortex_m/tz)

add_subdirectory_ifdef(CONFIG_ARM_MPU mpu)
add_subdirectory_ifdef(CONFIG_ARM_AARCH32_MMU mmu)

add_subdirectory_ifdef(CONFIG_CPU_AARCH32_CORTEX_R cortex_a_r)
add_subdirectory_ifdef(CONFIG_CPU_AARCH32_CORTEX_A cortex_a_r)

if (CONFIG_ARM_ZIMAGE_HEADER)
zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors zimage_header.ld)
zephyr_linker_sources(ROM_START SORT_KEY 0x1vectors vector_table.ld)
zephyr_linker_sources(ROM_START SORT_KEY 0x2vectors cortex_m/vector_table_pad.ld)
else()
zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors vector_table.ld)
zephyr_linker_sources(ROM_START SORT_KEY 0x1vectors cortex_m/vector_table_pad.ld)
endif()
