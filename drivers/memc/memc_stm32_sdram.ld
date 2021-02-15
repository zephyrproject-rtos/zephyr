/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram1), okay)
GROUP_START(SDRAM1)

	SECTION_PROLOGUE(_STM32_SDRAM1_SECTION_NAME, (NOLOAD),)
	{
		*(.stm32_sdram1)
		*(".stm32_sdram1.*")
	} GROUP_LINK_IN(SDRAM1)

GROUP_END(SDRAM1)
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram2), okay)
GROUP_START(SDRAM2)

	SECTION_PROLOGUE(_STM32_SDRAM2_SECTION_NAME, (NOLOAD),)
	{
		*(.stm32_sdram2)
		*(".stm32_sdram2.*")
	} GROUP_LINK_IN(SDRAM2)

GROUP_END(SDRAM2)
#endif
