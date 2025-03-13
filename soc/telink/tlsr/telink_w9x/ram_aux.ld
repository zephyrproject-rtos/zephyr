/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_HAS_CHOSEN(zephyr_aux_ram)

SECTION_DATA_PROLOGUE(data_aux,,)
{
	. = ALIGN(4);
	*(.data_aux)
	*(".data_aux.*")

	PROVIDE (_DATA_AUX_VMA_END = .);
	PROVIDE (_DATA_AUX_VMA_START = ADDR(data_aux));
	PROVIDE (_DATA_AUX_LMA_START = LOADADDR(data_aux));
} GROUP_DATA_LINK_IN(RAM_AUX, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(bss_aux,(NOLOAD),)
{
	. = ALIGN(4);
	*(.bss_aux)
	*(".bss_aux.*")

	PROVIDE (_BSS_AUX_VMA_END = .);
	PROVIDE (_BSS_AUX_VMA_START = ADDR(bss_aux));
} GROUP_DATA_LINK_IN(RAM_AUX, RAM_AUX)

#endif /* DT_HAS_CHOSEN(zephyr_aux_ram) */
