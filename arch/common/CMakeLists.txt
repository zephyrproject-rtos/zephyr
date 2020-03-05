# SPDX-License-Identifier: Apache-2.0

if(CONFIG_GEN_ISR_TABLES OR CONFIG_EXECUTION_BENCHMARKING)
  zephyr_library()

  zephyr_library_sources_ifdef(
    CONFIG_GEN_ISR_TABLES
    isr_tables.c
    sw_isr_common.c
    )

  zephyr_library_sources_ifdef(
    CONFIG_EXECUTION_BENCHMARKING
    timing_info_bench.c
    )
endif()

# Put functions and data in their own binary sections so that ld can
# garbage collect them
zephyr_cc_option(-ffunction-sections -fdata-sections)

zephyr_linker_sources_ifdef(CONFIG_GEN_ISR_TABLES
  SECTIONS
  ${ZEPHYR_BASE}/include/linker/intlist.ld
)

zephyr_linker_sources_ifdef(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
  RAM_SECTIONS
  ramfunc.ld
)

zephyr_linker_sources_ifdef(CONFIG_NOCACHE_MEMORY
  RAM_SECTIONS
  nocache.ld
)

# Only ARM, X86 and OPENISA_RV32M1_RISCV32 use TEXT_SECTION_OFFSET.
if (DEFINED CONFIG_ARM OR DEFINED CONFIG_X86
    OR DEFINED CONFIG_SOC_OPENISA_RV32M1_RISCV32)
  zephyr_linker_sources(ROM_START SORT_KEY 0x0 text_section_offset.ld)
endif()
