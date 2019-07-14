/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2017 Intel Corporation. All rights reserved.
 *
 * Author: Rander Wang <rander.wang@intel.com>
 */

/*
 * Entry point from boot loader.
 * Fix link address of this entry to SOF_TEXT_START so that
 * it is easy for boot loader to jump to the baseFW becasue
 * the boot loader and baseFW are in different elf file.
 */


// Exports
.global _MainEntry


/**************************************************************************/

	.begin	literal_prefix	.MainEntry
	.section .MainEntry.text, "ax"

	.align	4
	.global	_MainEntry

_MainEntry:

	call0	_start

	.size	_MainEntry, . - _MainEntry

	.end	literal_prefix
