/*
 * Copyright (c) 2020 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/can.h>

u32_t can_mcan_get_core_clock(struct device *dev)
{
	u32_t core_clock = LL_RCC_GetFDCANClockFreq(LL_RCC_FDCAN_CLKSOURCE);
	u32_t dbrp, nbrp;

#if CONFIG_CAN_CKDIV != 0
	core_clock /= CONFIG_CAN_CKDIV * 2;
#endif

__weak void can_mcan_clock_enable()
{
	LL_RCC_SetFDCANClockSource(LL_RCC_FDCAN_CLKSOURCE_PCLK1);
	__HAL_RCC_FDCAN_CLK_ENABLE();

	FDCAN_CONFIG->CKDIV = CONFIG_CAN_CKDIV;
}
