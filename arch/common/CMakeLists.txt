# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_include_directories(include)

# Library may be empty due to kconfigs
zephyr_library_property(ALLOW_EMPTY TRUE)

if(CONFIG_GEN_ISR_TABLES)
  zephyr_library_sources(
    sw_isr_common.c
  )
  zephyr_library_sources_ifdef(
    CONFIG_DYNAMIC_INTERRUPTS
    dynamic_isr.c
  )
endif()

zephyr_library_sources_ifdef(
  CONFIG_MULTI_LEVEL_INTERRUPTS
  multilevel_irq.c
  )

zephyr_library_sources_ifdef(CONFIG_SHARED_INTERRUPTS shared_irq.c)

if(NOT CONFIG_ARCH_HAS_TIMING_FUNCTIONS AND
    NOT CONFIG_SOC_HAS_TIMING_FUNCTIONS AND
    NOT CONFIG_BOARD_HAS_TIMING_FUNCTIONS)
zephyr_library_sources_ifdef(CONFIG_TIMING_FUNCTIONS timing.c)
endif()

# Put functions and data in their own binary sections so that ld can
# garbage collect them
zephyr_cc_option(-ffunction-sections -fdata-sections)

zephyr_linker_sources_ifdef(CONFIG_GEN_ISR_TABLES
  SECTIONS
  ${ZEPHYR_BASE}/include/zephyr/linker/intlist.ld
)

zephyr_linker_sources_ifdef(CONFIG_ISR_TABLES_LOCAL_DECLARATION
  SECTIONS
  ${ZEPHYR_BASE}/include/zephyr/linker/isr-local-drop-unused.ld
)

zephyr_linker_sources_ifdef(CONFIG_GEN_IRQ_VECTOR_TABLE
  ROM_START
  SORT_KEY 0x0vectors
  ${ZEPHYR_BASE}/include/zephyr/linker/irq-vector-table-section.ld
)

if(CONFIG_GEN_ISR_TABLES)
  zephyr_linker_section(NAME .intList VMA IDT_LIST LMA IDT_LIST NOINPUT PASS NOT LINKER_ZEPHYR_FINAL)
  zephyr_linker_section_configure(SECTION .intList KEEP INPUT ".irq_info" FIRST)
  zephyr_linker_section_configure(SECTION .intList KEEP INPUT ".intList")

  zephyr_linker_section_configure(SECTION /DISCARD/ KEEP INPUT ".irq_info" PASS LINKER_ZEPHYR_FINAL)
  zephyr_linker_section_configure(SECTION /DISCARD/ KEEP INPUT ".intList"  PASS LINKER_ZEPHYR_FINAL)
endif()

zephyr_linker_sources_ifdef(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
  RAM_SECTIONS
  ramfunc.ld
)

zephyr_linker_sources_ifdef(CONFIG_NOCACHE_MEMORY
  RAM_SECTIONS
  nocache.ld
)

# Only ARM, X86 and OPENISA_RV32M1_RISCV32 use ROM_START_OFFSET.
if (DEFINED CONFIG_ARM OR DEFINED CONFIG_X86 OR DEFINED CONFIG_ARM64
    OR DEFINED CONFIG_SOC_OPENISA_RV32M1_RISCV32)
  # Exclamation mark is printable character with lowest number in ASCII table.
  # We are sure that this file will be included as a first.
  zephyr_linker_sources(ROM_START SORT_KEY ! rom_start_address.ld)
  # Some linkers fill unspecified region with pattern other than 0x00. Include
  # fill_with_zeros.ld file which forces the linker to use 0x00 pattern. Please
  # note that the pattern will affect empty spaces created after FILL(0x00).
  zephyr_linker_sources(ROM_START SORT_KEY $ fill_with_zeros.ld)
  zephyr_linker_sources(ROM_START SORT_KEY 0x0 rom_start_offset.ld)
  # Handled in ld.cmake
endif()


# isr_tables is a normal CMake library and not a zephyr_library because it
# should not be --whole-archive'd
if (CONFIG_GEN_ISR_TABLES)
  add_library(isr_tables
    isr_tables.c
  )

  add_dependencies(isr_tables zephyr_generated_headers)
  target_link_libraries(isr_tables zephyr_interface)
  zephyr_library_link_libraries(isr_tables)
endif()

if(CONFIG_COVERAGE)
  zephyr_compile_options($<TARGET_PROPERTY:compiler,coverage>)
  zephyr_link_libraries_ifndef(CONFIG_NATIVE_LIBRARY $<TARGET_PROPERTY:linker,coverage>)
endif()

zephyr_library_sources_ifdef(CONFIG_SEMIHOST semihost.c)
