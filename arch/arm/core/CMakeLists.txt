zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            exc_exit.S
            irq_init.c
            swap.c
            swap_helper.S
            fault.c
            irq_manage.c
            thread.c
            cpu_idle.S
            fault_s.S
            fatal.c
            sys_fatal_error_handler.c
            thread_abort.c
            IFDEF:${CONFIG_GEN_SW_ISR_TABLE} isr_wrapper.S
            IFDEF:${CONFIG_CPLUSPLUS} __aeabi_atexit.c
            IFDEF:${CONFIG_IRQ_OFFLOAD} irq_offload.c
            IFDEF:${CONFIG_CPU_CORTEX_M0} irq_relay.S
            IFDEF:${CONFIG_USERSPACE} userspace.S
)

target_sources(arch_arm PRIVATE ${PRIVATE_SOURCES})

include_relative_ifdef(CONFIG_CPU_CORTEX_M cortex_m/CMakeLists.txt)
include_relative_ifdef(CONFIG_ARM_MPU cortex_m/mpu/CMakeLists.txt)
include_relative_ifdef(CONFIG_CPU_CORTEX_M_HAS_CMSE cortex_m/cmse/CMakeLists.txt)
include_relative_ifdef(CONFIG_ARM_SECURE_FIRMWARE cortex_m/tz/CMakeLists.txt)
