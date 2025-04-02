/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(psram), okay)
GROUP_START(PSRAM)

	SECTION_PROLOGUE(_STM32_PSRAM_SECTION_NAME, (NOLOAD),)
	{
		*(.stm32_psram)
		*(".stm32_psram.*")
	} GROUP_LINK_IN(PSRAM)

GROUP_END(PSRAM)
#endif
