_vector_start = .;
KEEP(*(.exc_vector_table))
KEEP(*(.exc_vector_table.*))

#if LINKER_ZEPHYR_FINAL && defined(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
INCLUDE isr_tables_vt.ld
#else
KEEP(*(.vectors))
#endif

_vector_end = .;
