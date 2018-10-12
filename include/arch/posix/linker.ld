/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the POSIX (native) platform
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

SECTIONS
 {
	SECTION_PROLOGUE(.note.ABI-tag,,)
	{
	*(.note.ABI-tag)
	}

 } INSERT AFTER .interp;

SECTIONS
 {

#include <linker/rel-sections.ld>

#include <linker/common-rom.ld>

#include <linker/common-ram.ld>

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
	/*
	 * This section is used for non-initialized objects that
	 * will not be cleared during the boot process.
	 */
	KERNEL_INPUT_SECTION(.noinit)
	KERNEL_INPUT_SECTION(".noinit.*")
	*(".kernel_noinit.*")
	}

#include <arch/posix/native_tasks.ld>

	/* Related to transactional memory */
	SECTION_PROLOGUE(.tm_clone_table,,)
	{
	*(.tm_clone_table)
	}

 __data_ram_end = .;

 } INSERT AFTER .data;

/*
 * Note that the INSERT command actually changes the meaning of the -T command
 * line switch: The script will now augment the default SECTIONS instead of
 * replacing it.
 */
