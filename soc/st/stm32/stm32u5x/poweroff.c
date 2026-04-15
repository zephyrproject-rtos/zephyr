/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/misc/stm32_wkup_pins/stm32_wkup_pins.h>

#include <stm32_common.h>
#include <stm32_ll_rtc.h>
#include <stm32_ll_pwr.h>

#if defined(CONFIG_RTC)
static bool stm32u5_rtc_alarm_internal_wakeup_enabled(void)
{
	return ((RTC->CR & RTC_CR_ALRAE) != 0U) && ((RTC->CR & RTC_CR_ALRAIE) != 0U);
}
#endif

void z_sys_poweroff(void)
{
#ifdef CONFIG_STM32_WKUP_PINS
	stm32_pwr_wkup_pin_cfg_pupd();
#endif /* CONFIG_STM32_WKUP_PINS */

#if defined(CONFIG_SOC_SERIES_STM32U5X) && defined(CONFIG_RTC)
	if (stm32u5_rtc_alarm_internal_wakeup_enabled()) {
		LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN7);
		LL_PWR_SetWakeUpPinSignal3Selection(LL_PWR_WAKEUP_PIN7);
	}
#endif

	LL_PWR_ClearFlag_SB();
	LL_PWR_ClearFlag_WU();
	LL_PWR_SetPowerMode(LL_PWR_SHUTDOWN_MODE);

	stm32_enter_poweroff();
}
