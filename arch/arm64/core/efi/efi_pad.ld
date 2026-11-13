/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Last section of zephyr.bin. Pads the file so the PE section size is a
 * multiple of the 0x200 FileAlignment in header.S. Without the BYTE(0),
 * the section holds no data and the file is not padded.
 *
 * Sections placed after noinit, with zephyr_linker_sources(SECTIONS), are
 * in zephyr.bin too: the padding then follows them at its link address.
 * Otherwise it is loaded right after the data, since the file holds nothing
 * for bss and noinit.
 */
.efi_pad : AT(__efi_pad_lma)
{
	BYTE(0)
	. += __efi_pad_size;
} > RAMABLE_REGION

__efi_pad_lma = ABSOLUTE((ADDR(.efi_pad) > ADDR(noinit) + SIZEOF(noinit)) ?
			 ADDR(.efi_pad) : __data_region_end);
__efi_pad_size = ABSOLUTE(0x1ff - ((__efi_pad_lma - __text_region_start) & 0x1ff));

/* End of zephyr.bin */
__efi_file_end = LOADADDR(.efi_pad) + SIZEOF(.efi_pad);
