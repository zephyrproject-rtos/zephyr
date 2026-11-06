/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o. o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-tool.h>

/*
 * The first 3504 bytes of DRAM are designated as shared memory between
 * application and RF cores.
 */
SECTION_PROLOGUE(.connectivity_shared_status_section, 0x28000000 (NOLOAD),)
{
	gFwStatusInfo = .;
	. = ALIGN(256);
} GROUP_NOLOAD_LINK_IN(DMEM_SHARED, DMEM_SHARED)

SECTION_PROLOGUE(.boot_report_shared_section, 0x28000100 (NOLOAD),)
{
	_Boot_report_address = .;
	. = ALIGN(3248);
} GROUP_NOLOAD_LINK_IN(DMEM_SHARED, DMEM_SHARED)
