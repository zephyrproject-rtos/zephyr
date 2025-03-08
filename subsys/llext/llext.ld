SECTIONS {
    .text : {
        *(.text .text.*);
    }

    .bss : SUBALIGN(8) {
        *(.bss .bss.*)
    }

    .data : SUBALIGN(8) {
        *(.data .data.*)
    }

    .rodata : SUBALIGN(8) {
        *(.rodata .rodata.*)
    }

    .export_sym : SUBALIGN(8) {
    	*(.export_sym)
    }
}
