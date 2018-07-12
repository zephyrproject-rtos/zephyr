zephyr_cc_option(-mlongcalls)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            cpu_idle.c
            fatal.c
            window_vectors.S
            IFDEF:${CONFIG_XTENSA_ASM2} xtensa-asm2-util.S
                                        xtensa-asm2.c
            IFDEF:${CONFIG_XTENSA_USE_CORE_CRT1} crt1.S
            IFDEF:${CONFIG_IRQ_OFFLOAD} irq_offload.c
            IFNDEF:${CONFIG_XTENSA_ASM2} xtensa_intr.c
                                         irq_manage.c
                                         swap.S
                                         thread.c
                                         xtensa_context.S
                                         xtensa_intr_asm.S
                                         xtensa_vectors.S
                                         xt_zephyr.S
            IFNDEF:${CONFIG_ATOMIC_OPERATIONS_C} atomic.S
)

target_sources(arch_xtensa PRIVATE ${PRIVATE_SOURCES})

include_relative(startup/CMakeLists.txt)
