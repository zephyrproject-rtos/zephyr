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
  isr_tables.ld
)

zephyr_linker_sources_ifdef(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT
  RAM_SECTIONS
  ramfunc.ld
)

zephyr_linker_sources_ifdef(CONFIG_NOCACHE_MEMORY
  RAM_SECTIONS
  nocache.ld
)
