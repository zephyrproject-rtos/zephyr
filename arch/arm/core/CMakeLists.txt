# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_SLOW_FLASH_DATA)
  if(COMPILER STREQUAL gcc)
    zephyr_compile_options(-mslow-flash-data -fno-jump-tables)
  elseif(COMPILER STREQUAL iar)
    zephyr_compile_options(--no_literal_pool --no_switch_tables)
  endif()
endif()

if(CONFIG_CPU_AARCH32_ARMV6)
  # ARM1176 / ARMv6KZ does NOT support true unaligned access -- a misaligned
  # LDR returns the aligned word rotated by (addr & 3) * 8 bits instead of
  # faulting (SCTLR.A=0, the default). GCC's default -munaligned-access
  # optimises portable byte-shift load idioms into a single LDR, which then
  # silently returns rotated data when the pointer is not 4-byte aligned.
  # Force byte loads, as Linux does for pre-ARMv7 in arch/arm/Makefile.
  zephyr_compile_options(-mno-unaligned-access)
endif()

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
zephyr_library_sources_ifdef(CONFIG_ARCH_STACKWALK stacktrace.c)

zephyr_cc_option_ifdef(CONFIG_ARCH_STACKWALK -funwind-tables)

add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M cortex_m)
add_subdirectory_ifdef(CONFIG_CPU_CORTEX_M_HAS_CMSE cortex_m/cmse)
add_subdirectory_ifdef(CONFIG_ARM_SECURE_FIRMWARE cortex_m/tz)
add_subdirectory_ifdef(CONFIG_ARM_NONSECURE_FIRMWARE cortex_m/tz)

add_subdirectory_ifdef(CONFIG_ARM_MPU mpu)
add_subdirectory_ifdef(CONFIG_ARM_AARCH32_MMU mmu)

add_subdirectory_ifdef(CONFIG_CPU_AARCH32_ARM9 arm9)
add_subdirectory_ifdef(CONFIG_CPU_AARCH32_ARMV6 armv6)
add_subdirectory_ifdef(CONFIG_CPU_AARCH32_CORTEX_R cortex_a_r)
add_subdirectory_ifdef(CONFIG_CPU_AARCH32_CORTEX_A cortex_a_r)

if(CONFIG_ARM_ZIMAGE_HEADER)
zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors zimage_header.ld)
zephyr_linker_sources(ROM_START SORT_KEY 0x1vectors vector_table.ld)
zephyr_linker_sources(ROM_START SORT_KEY 0x2vectors cortex_m/vector_table_pad.ld)
else()
zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors vector_table.ld)
zephyr_linker_sources(ROM_START SORT_KEY 0x1vectors cortex_m/vector_table_pad.ld)
endif()

if(CONFIG_GEN_SW_ISR_TABLE)
  if(CONFIG_DYNAMIC_INTERRUPTS)
    zephyr_linker_sources(RWDATA swi_tables.ld)
  else()
    zephyr_linker_sources(RODATA swi_tables.ld)
  endif()
endif()
