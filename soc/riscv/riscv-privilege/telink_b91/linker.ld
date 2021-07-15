/*
 * Copyright (c) 2021 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Telink B91 SoC
 */

#include <soc.h>
#include <autoconf.h>
#include <linker/linker-tool.h>

MEMORY
{
	ROM_INIT (rx)  : ORIGIN = DT_REG_ADDR(DT_CHOSEN(zephyr_flash)), LENGTH = DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
	RAM_ILM  (rwx) : ORIGIN = DT_REG_ADDR(DT_NODELABEL(ram_ilm)), LENGTH = DT_REG_SIZE(DT_NODELABEL(ram_ilm))
}

SECTIONS
{
	SECTION_PROLOGUE(vector,,)
	{
		. = ALIGN(4);
		KEEP(*(.init.*))
	} GROUP_LINK_IN(ROM_INIT)
}

#include <arch/riscv/common/linker.ld>

SECTIONS
{
	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		. = ALIGN(8);
		PROVIDE (__global_pointer$ = __data_ram_start + 0x800);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(retention_data,,)
	{
		. = ALIGN(8);
		*(.retention_data)
		*(".retention_data.*")

		PROVIDE (_RETENTION_DATA_VMA_END = .);
		PROVIDE (_RETENTION_DATA_VMA_START = ADDR(retention_data));
		PROVIDE (_RETENTION_DATA_LMA_START = LOADADDR(retention_data));
	} GROUP_DATA_LINK_IN(RAM_ILM, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(ram_code,,)
	{
		. = ALIGN(8);
		*(.ram_code)
		*(".ram_code.*")

		PROVIDE (_RAMCODE_VMA_END = .);
		PROVIDE (_RAMCODE_VMA_START = ADDR(ram_code));
		PROVIDE (_RAMCODE_LMA_START = LOADADDR(ram_code));
	} GROUP_DATA_LINK_IN(RAM_ILM, ROMABLE_REGION)
}
