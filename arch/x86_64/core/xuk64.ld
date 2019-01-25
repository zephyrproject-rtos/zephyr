SECTIONS {
	. = 0x100000;

	.text : {
	      *(.xuk_stub32)
	      . = ALIGN(16);
	      *(.xuk_start64*)
	      *(.text*)
	}
	.rodata : { *(.rodata*) }
	.data : { *(.data*) }
	__bss_start = .;
	.bss : { *(.bss*) *(COMMON) }
	__bss_end = .;
}
