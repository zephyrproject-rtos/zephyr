/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

SECTIONS
    {

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

#ifdef CONFIG_OPENAMP_RSC_TABLE

	SECTION_PROLOGUE(.resource_table,, SUBALIGN(4))
	{
		KEEP(*(.resource_table*))
    	} GROUP_LINK_IN(ROMABLE_REGION)
#endif
}
