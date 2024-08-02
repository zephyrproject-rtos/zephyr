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

#include <zephyr/linker/sections.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>


SECTIONS
{

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

SECTION_PROLOGUE(rom_start,,)
{
	/* Located in generated directory. This file is populated by the
	 * zephyr_linker_sources() Cmake function.
	 */
	#include <snippets-rom-start.ld>
} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/common-rom.ld>
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>

SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
{
	/* Located in generated directory. This file is populated by the
	 * zephyr_linker_sources() Cmake function.
	 */
	#include <snippets-rodata.ld>
} GROUP_LINK_IN(ROMABLE_REGION)

SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
{
	/* Located in generated directory. This file is populated by the
	 * zephyr_linker_sources() Cmake function.
	 */
	#include <snippets-rwdata.ld>
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <zephyr/linker/common-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#include <zephyr/arch/posix/native_tasks.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

__data_region_end = .;

SECTION_DATA_PROLOGUE(_NOINIT_SECTION_NAME,,)
{
	/* Located in generated directory. This file is populated by the
	 * zephyr_linker_sources() Cmake function.
	 */
	#include <snippets-noinit.ld>
} GROUP_LINK_IN(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

} INSERT AFTER .data;

SECTIONS
{
#include <zephyr/arch/posix/native_sim_interface.ld>
} INSERT AFTER .text;

/*
 * Note that the INSERT command actually changes the meaning of the -T command
 * line switch: The script will now augment the default SECTIONS instead of
 * replacing it.
 */
