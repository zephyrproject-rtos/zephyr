/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Companion to quick_access_code.ld: collect the MCUX HAL
 * "DataQuickAccess" input section (AT_QUICKACCESS_SECTION_DATA) so it is
 * not left as an orphan output section. Placed in the initialized data
 * region alongside the rest of .data.
 */
SECTION_DATA_PROLOGUE(.data_quick_access,,)
{
	. = ALIGN(4);
	KEEP(*(DataQuickAccess))
	. = ALIGN(4);
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
