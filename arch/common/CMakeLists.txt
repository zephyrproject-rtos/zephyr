# Put functions and data in their own binary sections so that ld can
# garbage collect them
zephyr_cc_option(-ffunction-sections -fdata-sections)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_GEN_ISR_TABLES}         isr_tables.c
                                                   sw_isr_common.c
            IFDEF:${CONFIG_EXECUTION_BENCHMARKING} timing_info_bench.c
)

target_sources(arch_${ARCH} PRIVATE ${PRIVATE_SOURCES})
