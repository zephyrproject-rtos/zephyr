/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

GROUP_START(BACKUP_SRAM)

	SECTION_PROLOGUE(_STM32_BACKUP_SRAM_SECTION_NAME, (NOLOAD),)
	{
		*(.stm32_backup_sram)
		*(".stm32_backup_sram.*")
	} GROUP_LINK_IN(BACKUP_SRAM)

GROUP_END(BACKUP_SRAM)
