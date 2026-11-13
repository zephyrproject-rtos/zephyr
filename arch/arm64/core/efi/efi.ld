/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Values for the PE/COFF header in header.S. The file holds everything from
 * __text_region_start to __efi_file_end, set in efi_pad.ld. The first 4 KiB,
 * which hold the image and PE headers, are the PE "headers". The rest is a
 * single section.
 */
__efi_entry_offset = arm64_efi_entry - __text_region_start;
__efi_section_size = __efi_file_end - __text_region_start - 0x1000;
__efi_image_size = ALIGN(__efi_file_end - __text_region_start, 0x1000);

ASSERT(__efi_entry_offset >= 0x1000, "EFI entry point within the PE headers")
