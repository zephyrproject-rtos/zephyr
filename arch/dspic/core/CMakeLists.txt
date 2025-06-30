zephyr_library()

zephyr_library_sources(
    cpu_idle.c
    fatal.c
    irq_manage.c
    isr_wrapper.c
    prep_c.c
    thread.c
    swap.c
    tls.c
    reset0.S
    init.S
    vector_table.S
    reset1.S
)

zephyr_linker_sources(ROM_START SORT_KEY 0x04 vector_table.ld)
