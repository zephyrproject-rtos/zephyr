/*
 * Copyright (c) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(FLASH_CCFG)

SECTION_PROLOGUE(.ti_ccfg, , ){
	KEEP(*(.ccfg))} GROUP_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(ti_ccfg_partition)))

	GROUP_END(FLASH_CCFG)
