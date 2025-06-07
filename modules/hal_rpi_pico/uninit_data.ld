.uninitialized_data (NOLOAD): {
    . = ALIGN(4);
    *(.uninitialized_data*)
} > RAM
