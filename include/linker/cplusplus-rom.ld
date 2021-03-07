/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined (CONFIG_CPLUSPLUS)
	SECTION_PROLOGUE(.gcc_except_table,,ONLY_IF_RO)
	{
	*(.gcc_except_table .gcc_except_table.*)
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif
