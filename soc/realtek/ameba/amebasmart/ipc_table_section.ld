/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cortex-M IPC dispatcher table: ipc_table_init() walks the IPC_INIT_TABLE
 * entries prebuilt libraries emit. CA32 has its copy in boot_section.ld.
 */
SECTION_DATA_PROLOGUE(ipc_table,,)
{
	. = ALIGN(4);
	__ipc_table_start__ = .;
	KEEP(*(SORT(.ipc.table.data*)))
	__ipc_table_end__ = .;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
