/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm64/scripts/linker.ld>

SECTIONS
{
#ifdef CONFIG_OPENAMP_RSC_TABLE
	SECTION_PROLOGUE(.resource_table,, SUBALIGN(8))
	{
		KEEP(*(.resource_table*))
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif
}
