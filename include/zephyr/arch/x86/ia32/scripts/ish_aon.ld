/*
 * Copyright (c) 2023-2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(ISH_AON)

	SECTION_PROLOGUE(aon,,)
	{
		__ish_aon_region_start = .;

		KEEP(*libintel_hal_aontask.a:*(.text.*))
		KEEP(*libintel_hal_aontask.a:*(.rodata.*))
		KEEP(*libintel_hal_aontask.a:*(.data.*))
		KEEP(*libintel_hal_aontask.a:*(.bss.*))

		. = ALIGN(4);
		__ish_aon_region_end = .;
	} GROUP_LINK_IN(AON)

GROUP_END(ISH_AON)
