/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <arch/arm/cortex_m/scripts/linker.ld>

SECTIONS
    {

#include <linker/rel-sections.ld>
#ifdef CONFIG_RPROC_RSC_TABLE

	SECTION_PROLOGUE(.resource_table,, SUBALIGN(4))
	{
		KEEP(*(.resource_table*))
    	} GROUP_LINK_IN(ROMABLE_REGION)
#endif
}