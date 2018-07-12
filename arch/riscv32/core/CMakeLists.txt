zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            cpu_idle.c
            fatal.c
            irq_manage.c
            irq_offload.c
            isr.S
            prep_c.c
            reset.S
            swap.S
            thread.c
)

target_sources(arch_riscv32 PRIVATE ${PRIVATE_SOURCES})
