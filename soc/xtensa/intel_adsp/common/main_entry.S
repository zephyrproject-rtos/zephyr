/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright(c) 2017 Intel Corporation. All rights reserved.
 *
 * Author: Rander Wang <rander.wang@intel.com>
 */

/*
 * Entry point from boot loader.
 * Fix link address of this entry to SOF_TEXT_START so that
 * it is easy for boot loader to jump to the baseFW because
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

#ifdef CONFIG_RESET_VECTOR_IN_BOOTLOADER
	j	_start
#else
	j	__start
#endif

	.size	_MainEntry, . - _MainEntry

	.end	literal_prefix
