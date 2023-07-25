/*
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-tool.h>

#define IS_CHOSEN_SRAM(x) (DT_DEP_ORD(DT_NODELABEL(x)) == DT_DEP_ORD(DT_CHOSEN(zephyr_sram)))

#if (DT_REG_SIZE(DT_NODELABEL(ocram)) > 0) && !IS_CHOSEN_SRAM(ocram)
GROUP_START(OCRAM)
	__ocram_start = .;
	SECTION_PROLOGUE(.ocram_bss,(NOLOAD),SUBALIGN(4))
	{
		__ocram_bss_start = .;
		*(.ocram_bss)
		*(".ocram_bss.*")
		__ocram_bss_end = .;
		__ocram_end = .;
	} GROUP_LINK_IN(OCRAM)
	SECTION_PROLOGUE(.ocram_noinit,(NOLOAD),SUBALIGN(4))
	{
		__ocram_noinit_start = .;
		*(.ocram_noinit)
		*(".ocram_noinit.*")
		__ocram_noinit_end = .;
	} GROUP_LINK_IN(OCRAM)
	SECTION_PROLOGUE(.ocram_data,,SUBALIGN(4))
	{
		__ocram_data_start = .;
		*(.ocram_data)
		*(".ocram_data.*")
		__ocram_data_end = .;
	} GROUP_LINK_IN(OCRAM AT> ROMABLE_REGION)
	__ocram_end = .;

	__ocram_data_load_start = LOADADDR(.ocram_data);
#endif
