/* Copyright (C) 2023 BeagleBoard.org Foundation
 * Copyright (C) 2023 S Prashanth
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/arm/cortex_a_r/scripts/linker.ld>

#ifdef CONFIG_R5_BOOT_SPRINGBOARD
ENTRY(__boot_spring)
#endif

SECTIONS
{
#ifdef CONFIG_OPENAMP_RSC_TABLE
	SECTION_PROLOGUE(.resource_table,, SUBALIGN(4))
	{
		KEEP(*(.resource_table*))
	} GROUP_LINK_IN(RSC_TABLE)
#endif

#ifdef CONFIG_R5_BOOT_SPRINGBOARD
	SECTION_PROLOGUE(boot_start,, SUBALIGN(4))
	{
		KEEP(*(.boot_section._start))
		KEEP(*(.boot_section*))
	} GROUP_LINK_IN(ATCM)
#endif
}
