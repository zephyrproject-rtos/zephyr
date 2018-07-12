
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            thread.c
            cpu_idle.c
            fatal.c
            irq_manage.c
            swap.S
            prep_c.c
            reset.S
            cache.c
            exception.S
            crt0.S
            IF_KCONFIG irq_offload.c
)

target_sources(arch_nios2 PRIVATE ${PRIVATE_SOURCES})
target_link_libraries(arch_nios2 INTERFACE -u_exception_enter_fault -uon_irq_stack)
