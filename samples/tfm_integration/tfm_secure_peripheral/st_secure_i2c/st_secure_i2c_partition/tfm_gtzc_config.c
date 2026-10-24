/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "stm32u5xx_hal.h"
#include "target_cfg.h"

extern void Error_Handler(void);

#define SECURE_PERIPH_ATTRIBUTES (GTZC_TZSC_PERIPH_SEC | GTZC_TZSC_PERIPH_NPRIV)

static void configure_secure_peripheral(uint32_t peripheral)
{
	uint32_t attributes = 0U;

	if ((HAL_GTZC_TZSC_ConfigPeriphAttributes(peripheral, SECURE_PERIPH_ATTRIBUTES) !=
	     HAL_OK) ||
	    (HAL_GTZC_TZSC_GetConfigPeriphAttributes(peripheral, &attributes) != HAL_OK) ||
	    (attributes != SECURE_PERIPH_ATTRIBUTES)) {
		Error_Handler();

		for (;;) {
			__NOP();
		}
	}
}

void tfm_platform_gtzc_pre_lock_config(void)
{
	configure_secure_peripheral(GTZC_PERIPH_TIM6);
	configure_secure_peripheral(GTZC_PERIPH_I2C2);
}
