/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telink_tlx_regions.ld"

/*
 * Moving BLE library sections (without suffix) to RAM DLM (non-retention)
 */
SECTION_DATA_PROLOGUE(ram_dlm_data,,)
{
	. = ALIGN(4);
	*(.data)
	*(.sdata)

	PROVIDE (_RAM_DLM_DATA_VMA_END = .);
	PROVIDE (_RAM_DLM_DATA_VMA_START = ADDR(ram_dlm_data));
	PROVIDE (_RAM_DLM_DATA_LMA_START = LOADADDR(ram_dlm_data));
} GROUP_DATA_LINK_IN(TELINK_RAM_DATA_REGION, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(ram_dlm_bss,(NOLOAD),)
{
	. = ALIGN(4);
	*(.bss)
	*(".bss.*")
	COMMON_SYMBOLS

	/* Filter .noinit and .bss from mempool.c.obj [kheap__system_heap|len=0x1000]
	 to move from ilm to dlm after config LZMA */
	"*mempool.c.obj*"(".noinit.*" ".bss.*")

	/* Moving the BLE controller stack (HAL) */
	"*tlx_bt.c.obj*"(".noinit.*" ".bss.*")

	/* Moving the Zephyr BLE data */
	"*att.c.obj*"(".noinit.*" ".bss.*")
	"*buf.c.obj*"(".noinit.*")
	"*conn.c.obj*"(".noinit.*" ".bss.*")
	"*crypto.c.obj*"(".bss.*")
	"*gatt.c.obj*"(".bss.*")
	"*hci_core.c.obj*"(".noinit.*" ".bss.*")
	"*l2cap.c.obj*"(".bss.*")

	PROVIDE (_RAM_DLM_BSS_VMA_END = .);
	PROVIDE (_RAM_DLM_BSS_VMA_START = ADDR(ram_dlm_bss));
	PROVIDE (_RAM_DLM_BSS_LMA_START = LOADADDR(ram_dlm_bss));
} GROUP_DATA_LINK_IN(TELINK_RAM_DATA_REGION, TELINK_RAM_DATA_REGION)

/*
 * Moving BLE relative ramcode to special section
 */
SECTION_DATA_PROLOGUE(ram_code_ble,,)
{
	. = ALIGN(4);
	*(.ram_code_ble)
	*(".ram_code_ble.*")
	"*tlx_bt.c.obj*"(.ram_code)

	PROVIDE (_RAMCODE_BLE_VMA_END = .);
	PROVIDE (_RAMCODE_BLE_VMA_START = ADDR(ram_code_ble));
	PROVIDE (_RAMCODE_BLE_LMA_START = LOADADDR(ram_code_ble));
} GROUP_DATA_LINK_IN(TELINK_RAM_CODE_BLE_REGION, ROMABLE_REGION)
