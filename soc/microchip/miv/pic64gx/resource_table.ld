/* linker.ld - Linker command/script file
 *
 * Copyright (c) 2023 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.resource_table,, SUBALIGN(0x10))
{
	__rsctable_start = .;
	KEEP(*(.resource_table*))
	__rsctable_end = .;
#if CONFIG_PIC64GX_RELOCATE_RESOURCE_TABLE
} > LINKER_DT_NODE_REGION_NAME(DT_CHOSEN(zephyr_rsctable_mem)) AT > ROMABLE_REGION
#else
} GROUP_LINK_IN(ROMABLE_REGION)
#endif
__rsctable_load_start = LOADADDR(.resource_table);
