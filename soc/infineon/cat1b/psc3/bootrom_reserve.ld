/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The boot ROM uses the first 8 KiB of SRAM as scratch on every reset,
 * including the DeepSleep-RAM (suspend-to-RAM) warm-boot reset.  On a cold
 * boot this is harmless because .ramfunc/.data are re-copied and .bss is
 * re-zeroed during initialization.  An S2RAM warm resume, however, returns
 * directly to the suspend point and skips that initialization, so anything the
 * image links into the first 8 KiB (including the kernel state, and therefore
 * _current) would be left holding boot ROM scratch and the first scheduler
 * access would fault.
 *
 * Reserve that region here so .ramfunc, .data, .bss and noinit are all placed
 * above it and survive the warm boot unmodified, with no copying on resume.
 * This snippet is emitted into the RAM_SECTIONS hook, immediately after
 * _image_ram_start and ahead of the .ramfunc section (the SORT_KEY orders it
 * before the default entries).  The reservation is only made when S2RAM is
 * enabled, so builds that reinitialize RAM on every boot do not lose the 8 KiB.
 *
 * The reservation must be made from inside an output section: the RAM_SECTIONS
 * hook links into RAMABLE_REGION, and a bare location-counter assignment at the
 * region level does not advance the region's allocation pointer, so the next
 * '> RAMABLE_REGION' section (.ramfunc) would still be placed at the region
 * base.  A NOLOAD section that advances the location counter by 8 KiB reserves
 * the space without consuming any load (ROM) image.
 */
#if defined(CONFIG_PM_S2RAM)
SECTION_PROLOGUE(.ifx_bootrom_reserve, (NOLOAD),)
{
	_ifx_bootrom_reserve_start = .;
	. += 0x2000;
	_ifx_bootrom_reserve_end = .;
} GROUP_LINK_IN(RAMABLE_REGION)
#endif
