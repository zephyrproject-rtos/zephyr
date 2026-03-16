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
#include <zephyr/drivers/misc/stm32_wkup_pins/stm32_wkup_pins.h>

#include <stm32_common.h>
#include <stm32_ll_pwr.h>

void z_sys_poweroff(void)
{
	LL_PWR_ClearFlag_WU();

	LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);

	stm32_enter_poweroff();
}
