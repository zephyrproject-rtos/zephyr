zephyr_library()

zephyr_library_sources(
    cpu_idle.c
    fatal.c
    irq_manage.c
    isr_wrapper.S
    prep_c.c
    thread.c
    swap.S
    tls.c
    reset0.S
    init.S
    vector_table.S
    reset1.S
    irq_offload.c
    thread_abort.c
)

zephyr_linker_sources(ROM_START SORT_KEY 0x00 vector_table.ld)
