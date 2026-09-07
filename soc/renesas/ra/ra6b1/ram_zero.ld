/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
SECTION_DATA_PROLOGUE(__ram_zero$$,(NOLOAD),SUBALIGN(4))
{
	__ram_zero$$Base = . ;
	/* section.ram.zero */
	*(.ram)
	*(.bss.shared_data)
        *(.bss.shared_ram_ipc)
	. = ALIGN(4);
	__ram_zero$$Limit = . ;
} GROUP_LINK_IN(RAMABLE_REGION)
