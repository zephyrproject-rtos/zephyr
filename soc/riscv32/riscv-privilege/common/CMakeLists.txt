zephyr_include_directories(.)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            idle.c
            soc_irq.S
            soc_common_irq.c
            vector.S
)

target_sources(soc_riscv32 PRIVATE ${PRIVATE_SOURCES})
