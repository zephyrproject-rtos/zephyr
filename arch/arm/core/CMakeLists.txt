# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  fatal.c
  nmi.c
  nmi_on_reset.S
)

zephyr_library_sources_ifdef(CONFIG_CPP __aeabi_atexit.c)
zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
zephyr_library_sources_ifdef(CONFIG_USERSPACE userspace.S)
zephyr_library_sources_ifdef(CONFIG_ARM_ZIMAGE_HEADER header.S)
zephyr_library_sources_ifdef(CONFIG_LLEXT elf.c)
zephyr_library_sources_ifdef(CONFIG_GDBSTUB gdbstub.c)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M cortex_m)
add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M_HAS_CMSE cortex_m/cmse)
add_subdirectory_ifdef(CONFIG_ARM_SECURE_FIRMWARE cortex_m/tz)
add_subdirectory_ifdef(CONFIG_ARM_NONSECURE_FIRMWARE cortex_m/tz)

add_subdirectory_ifdef(CONFIG_ARM_MPU mpu)
add_subdirectory_ifdef(CONFIG_ARM_AARCH32_MMU mmu)

add_subdirectory_ifdef(CONFIG_CPU_AARCH32_CORTEX_R cortex_a_r)
add_subdirectory_ifdef(CONFIG_CPU_AARCH32_CORTEX_A cortex_a_r)

if(CONFIG_GEN_SW_ISR_TABLE)
  if(CONFIG_DYNAMIC_INTERRUPTS)
    zephyr_linker_sources(RWDATA swi_tables.ld)
  else()
    zephyr_linker_sources(RODATA swi_tables.ld)
  endif()
endif()
