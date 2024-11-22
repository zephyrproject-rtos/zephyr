/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telink_tlx_regions.ld"

/*
 * Moving BLE library sections (without suffix) to RAM DLM (non-retention)
 */
SECTION_DATA_PROLOGUE(ram_non_ret,,)
{
	. = ALIGN(4);
	*(.data)
	*(.sdata)
	*(.bss)

	/* Moving the BLE controller stack (HAL) */
	"*b9x_bt.c.obj*"(".noinit.*" ".bss.*")

	/* Moving the Zephyr BLE data */
	"*att.c.obj*"(".noinit.*" ".bss.*")
	"*buf.c.obj*"(".noinit.*")
	"*conn.c.obj*"(".noinit.*" ".bss.*")
	"*crypto.c.obj*"(".bss.*")
	"*gatt.c.obj*"(".bss.*")
	"*hci_core.c.obj*"(".noinit.*" ".bss.*")
	"*l2cap.c.obj*"(".bss.*")

	PROVIDE (_RAM_NON_RET_VMA_END = .);
	PROVIDE (_RAM_NON_RET_VMA_START = ADDR(ram_non_ret));
	PROVIDE (_RAM_NON_RET_LMA_START = LOADADDR(ram_non_ret));
} GROUP_DATA_LINK_IN(TELINK_RAM_DATA_REGION, ROMABLE_REGION)

/*
 * Moving BLE relative ramcode to special section
 */
SECTION_DATA_PROLOGUE(ram_code_ble,,)
{
	. = ALIGN(4);
	*(.ram_code_ble)
	*(".ram_code_ble.*")
	"*b9x_bt.c.obj*"(.ram_code)

	PROVIDE (_RAMCODE_BLE_VMA_END = .);
	PROVIDE (_RAMCODE_BLE_VMA_START = ADDR(ram_code_ble));
	PROVIDE (_RAMCODE_BLE_LMA_START = LOADADDR(ram_code_ble));
} GROUP_DATA_LINK_IN(TELINK_RAM_CODE_REGION, ROMABLE_REGION)
