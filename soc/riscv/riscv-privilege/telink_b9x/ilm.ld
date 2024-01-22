/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telink_b9x_regions.ld"

SECTION_DATA_PROLOGUE(retention_reset,,)
{
	. = ALIGN(4);
	KEEP(*(.retention_reset.*))

	PROVIDE (_RETENTION_RESET_VMA_END = .);
	PROVIDE (_RETENTION_RESET_VMA_START = ADDR(retention_reset));
	PROVIDE (_RETENTION_RESET_LMA_START = LOADADDR(retention_reset));
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(aes_data,,)
{
	. = ALIGN(4);
	*(.aes_data)
	*(".aes_data.*")

	PROVIDE (_AES_DATA_VMA_END = .);
	PROVIDE (_AES_DATA_VMA_START = ADDR(aes_data));
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(retention_data,,)
{
	. = ALIGN(4);
	*(.retention_data)
	*(".retention_data.*")

	PROVIDE (_RETENTION_DATA_VMA_END = .);
	PROVIDE (_RETENTION_DATA_VMA_START = ADDR(retention_data));
	PROVIDE (_RETENTION_DATA_LMA_START = LOADADDR(retention_data));
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(ram_code_retention,,)
{
	. = ALIGN(4);
	*(.ram_code_retention)
	*(".ram_code_retention.*")

	PROVIDE (_RAMCODE_RET_VMA_END = .);
	PROVIDE (_RAMCODE_RET_VMA_START = ADDR(ram_code_retention));
	PROVIDE (_RAMCODE_RET_LMA_START = LOADADDR(ram_code_retention));
} GROUP_DATA_LINK_IN(TELINK_RAM_CODE_RET_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(ram_code,,)
{
	. = ALIGN(4);
	*(.ram_code)
	*(".ram_code.*")
	*(.ram_code_ble)
	*(".ram_code_ble.*")

	PROVIDE (_RAMCODE_VMA_END = .);
	PROVIDE (_RAMCODE_VMA_START = ADDR(ram_code));
	PROVIDE (_RAMCODE_LMA_START = LOADADDR(ram_code));
} GROUP_DATA_LINK_IN(TELINK_RAM_CODE_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(noinit_ilm, (NOLOAD),)
{
	. = ALIGN(4);
	*(.noinit)
	*(".noinit.*")
} GROUP_DATA_LINK_IN(TELINK_RAM_REGION, TELINK_RAM_REGION)
