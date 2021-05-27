/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined (CONFIG_CPLUSPLUS)
	SECTION_DATA_PROLOGUE(.gcc_except_table,,ONLY_IF_RW)
	{
	*(.gcc_except_table .gcc_except_table.*)
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined (CONFIG_EXCEPTIONS)
    SECTION_PROLOGUE(.eh_frame_hdr,,)
	{
	*(.eh_frame_hdr)
	}

    SECTION_PROLOGUE(.eh_frame,,)
	{
	KEEP (*(SORT_NONE(EXCLUDE_FILE (*crtend.o) .eh_frame)))
	KEEP (*(SORT_NONE(.eh_frame)))
	} GROUP_LINK_IN(ROMABLE_REGION)

    SECTION_PROLOGUE(.tm_clone_table,,)
	{
	KEEP (*(SORT_NONE(EXCLUDE_FILE (*crtend.o) .tm_clone_table)))
	KEEP (*(SORT_NONE(.tm_clone_table)))
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_EXCEPTIONS */

#endif /* CONFIG_CPLUSPLUS */
