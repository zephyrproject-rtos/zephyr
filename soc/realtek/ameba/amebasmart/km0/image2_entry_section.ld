/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The LP bootloader ignores the reset vector and enters via the Img2EntryFun0
 * struct, which must be the first bytes of image2 (ORIGIN(RAM), 0x23002020).
 */

	__image2_entry_func__ = .;
	KEEP(*(SORT(.image2.entry.data*)))
	__image2_entry_func_end__ = .;
	/* km0_boot_trampoline follows the table, matching the vendor layout. */
	. = ALIGN(4);
	KEEP(*(.image2.entry.text*))
