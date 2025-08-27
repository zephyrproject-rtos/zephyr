/*
 * Copyright (c) 2023-2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(ISH_AON)

	SECTION_PROLOGUE(aon,,)
	{
		__ish_aon_start = .;
		KEEP(*libish_aontask.a:*(.data.*))
		KEEP(*libish_aontask.a:*(.text.*))
		KEEP(*libish_aontask.a:*(.bss.*))
		__ish_aon_end = .;
	} GROUP_LINK_IN(AON)

GROUP_END(ISH_AON)
