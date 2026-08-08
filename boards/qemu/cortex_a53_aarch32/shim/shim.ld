/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Linker script for the AArch64 EL3 boot shim. QEMU loads the -bios image so
 * that its first byte is the reset entry point, so the shim is linked at 0 and
 * objcopy'd to a raw binary. Only .text is needed; the shim uses no writable
 * data and no stack.
 */

OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_shim_start)

SECTIONS
{
	. = 0x0;

	.text : {
		KEEP(*(.text._shim_start))
		*(.text*)
		*(.rodata*)
	}

	/DISCARD/ : {
		*(.comment)
		*(.note*)
		*(.eh_frame*)
	}
}
