zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            thread.c
            thread_entry_wrapper.S
            cpu_idle.S
            fatal.c
            fault.c
            fault_s.S
            irq_manage.c
            cache.c
            timestamp.c
            isr_wrapper.S
            regular_irq.S
            swap.S
            sys_fatal_error_handler.c
            prep_c.c
            reset.S
            vector_table.c
            IFDEF:${CONFIG_ARC_FIRQ} fast_irq.S
            IFDEF:${CONFIG_ATOMIC_OPERATIONS_CUSTOM} atomic.c
            IFDEF:${CONFIG_USERSPACE} userspace.S
            IF_KCONFIG irq_offload.c
)

target_sources(arch_arc PRIVATE ${PRIVATE_SOURCES})

include_relative_ifdef(CONFIG_ARC_CORE_MPU mpu/CMakeLists.txt)
