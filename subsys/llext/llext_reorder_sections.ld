SECTIONS {
    .pinned_text : {
        *(.pinned_text .pinned_text.*)
    }

    .text : {
        *(.text .text.*)
	}

    .bss : {
        *(.bss .bss.*)
    }

	.data : {
        *(.data .data.*)
	}

    .rodata_in_data : {
        *(.rodata_in_data .rodata_in_data.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }
}
