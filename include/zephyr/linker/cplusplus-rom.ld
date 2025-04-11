/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined (CONFIG_CPP)
	SECTION_PROLOGUE(.gcc_except_table,,ONLY_IF_RO)
	{
	*(.gcc_except_table .gcc_except_table.*)
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined (CONFIG_CPP_EXCEPTIONS)
	SECTION_PROLOGUE(.eh_frame_hdr,,)
	{
	PROVIDE(__eh_frame_hdr_start = SIZEOF(.eh_frame_hdr) > 0 ? ADDR(.eh_frame_hdr) : 0);
	*(.eh_frame_hdr)
	PROVIDE(__eh_frame_hdr_end = SIZEOF(.eh_frame_hdr) > 0 ? . : 0);
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_PROLOGUE(.eh_frame,,)
	{
        PROVIDE (__eh_frame_start = .);
	KEEP (*(SORT_NONE(EXCLUDE_FILE (*crtend.o) .eh_frame)))
	KEEP (*(SORT_NONE(.eh_frame)))
        PROVIDE (__eh_frame_end = .);
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_CPP_EXCEPTIONS */

#endif /* CONFIG_CPP */
