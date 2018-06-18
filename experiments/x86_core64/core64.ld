SECTIONS {
	. = 0x100000;

	.text : {
	      *(.stub32)
	      . = ALIGN(16);
	      *(.start64*)
	      *(.text*)
	}
	.rodata : { *(.rodata*) }
	.data : { *(.data*) }
	.bss : { *(.bss*) *(COMMON) }
}
