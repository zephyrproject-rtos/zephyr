/*
 * Copyright (c) 2025 Oleh Kravchenko <oleh@kaa.org.ua>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32f1xx_ll_cortex.h>
#include <stm32f1xx_ll_pwr.h>

#include <zephyr/drivers/misc/stm32_wkup_pins/stm32_wkup_pins.h>
#include <zephyr/sys/poweroff.h>

void z_sys_poweroff(void)
{
#ifdef CONFIG_STM32_WKUP_PINS
	stm32_pwr_wkup_pin_cfg_pupd();

	LL_PWR_ClearFlag_WU();
#endif /* CONFIG_STM32_WKUP_PINS */

	LL_LPM_DisableEventOnPend();
	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
	LL_LPM_EnableDeepSleep();

	k_cpu_idle();

	CODE_UNREACHABLE;
}
