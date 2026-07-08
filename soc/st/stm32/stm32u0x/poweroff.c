/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/misc/stm32_wkup_pins/stm32_wkup_pins.h>

#include <stm32_bitops.h>
#include <stm32_common.h>
#include <stm32_ll_pwr.h>

void z_sys_poweroff(void)
{
#ifdef CONFIG_STM32_WKUP_PINS
	stm32_pwr_wkup_pin_cfg_pupd();
#endif /* CONFIG_STM32_WKUP_PINS */

	stm32_reg_set_bits((volatile uint32_t *)PWR->SCR,
			   PWR_SCR_CWUF1 | PWR_SCR_CWUF2 | PWR_SCR_CWUF3 |
			   PWR_SCR_CWUF4 | PWR_SCR_CWUF5 | PWR_SCR_CWUF7);
	LL_PWR_SetPowerMode(LL_PWR_MODE_SHUTDOWN);

	stm32_enter_poweroff();
}
