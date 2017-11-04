# Put functions and data in their own binary sections so that ld can
# garbage collect them
zephyr_cc_option(-ffunction-sections -fdata-sections)

zephyr_sources_ifdef(
  CONFIG_GEN_ISR_TABLES
  isr_tables.c
  )

zephyr_sources_ifdef(
  CONFIG_EXECUTION_BENCHMARKING
  timing_info_bench.c
  )
