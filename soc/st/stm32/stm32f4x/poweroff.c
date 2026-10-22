/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2024 STMicroelectronics
 * Copyright (c) 2025 Tomas Jurena
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>

#include <stm32_bitops.h>
#include <stm32_common.h>
#include <stm32_gpio_shared.h>
#include <stm32_ll_pwr.h>

#if defined(LL_PWR_WAKEUP_PIN3)
#define WAKEUP_PINS (LL_PWR_WAKEUP_PIN1 | LL_PWR_WAKEUP_PIN2 | LL_PWR_WAKEUP_PIN3)
#elif defined(LL_PWR_WAKEUP_PIN2)
#define WAKEUP_PINS (LL_PWR_WAKEUP_PIN1 | LL_PWR_WAKEUP_PIN2)
#else
#define WAKEUP_PINS LL_PWR_WAKEUP_PIN1
#endif

void z_sys_poweroff(void)
{
	const uint32_t wakeup_pins = stm32_reg_read(&PWR->CSR) & WAKEUP_PINS;

	/*
	 * A wake-up source that is high while WUF is cleared can mask later
	 * wake-up events (ES0206 2.2.4), so clear WUF with the WKUP pins
	 * disabled. A pin that is still high sets WUF again when re-enabled, and
	 * the device then leaves Standby at once.
	 */
	LL_PWR_DisableWakeUpPin(wakeup_pins);
	LL_PWR_ClearFlag_WU();
	LL_PWR_EnableWakeUpPin(wakeup_pins);

	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);

	stm32_enter_poweroff();
}
