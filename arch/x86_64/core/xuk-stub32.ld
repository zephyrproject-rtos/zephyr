/* SPDX-License-Identifier: Apache-2.0 */

ENTRY(_start)

PHDRS {
      stub32 PT_LOAD;
}

SECTIONS {
	. = 0x100000;
	_start = .;
	.stub32 : {
		*(.start32)
		*(.text*)
		*(.rodata*)
		*(.data*)
		*(.bss*)
		*(COMMON)
	} :stub32

	. = ALIGN(16);
	_start64 = .;
}
