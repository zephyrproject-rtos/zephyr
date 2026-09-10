/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmsis_core.h>
#include <stm32_ll_pwr.h>
#include <stm32_bitops.h>

void pm_s2ram_mark_set(void)
{
	/* Empty: the standby flag is set by hardware when exiting from standby. */
}

bool pm_s2ram_mark_check_and_clear(void)
{
	bool boot_after_standby = false;

	__HAL_RCC_PWR_CLK_ENABLE();

	if (LL_PWR_IsActiveFlag_SB() && (stm32_reg_read(&RCC->CSR) == 0U)) {
		LL_PWR_ClearFlag_SB();
		boot_after_standby = true;
	}

	return boot_after_standby;
}
