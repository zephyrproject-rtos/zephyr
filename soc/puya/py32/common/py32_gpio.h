/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_PUYA_PY32_GPIO_H_
#define ZEPHYR_SOC_PUYA_PY32_GPIO_H_

#include <soc.h>
#include <py32f0xx_ll_gpio.h>
#include <zephyr/irq.h>

/* Configure shared GPIO registers atomically; set output data before the mode. */
static inline void py32_gpio_configure(GPIO_TypeDef *gpio, uint32_t pin, uint32_t mode,
				       uint32_t pull, uint32_t speed, bool open_drain, uint32_t af,
				       bool output_low, bool output_high)
{
	uint32_t mask = BIT(pin);
	unsigned int key = irq_lock();

	if (output_high) {
		LL_GPIO_SetOutputPin(gpio, mask);
	} else if (output_low) {
		LL_GPIO_ResetOutputPin(gpio, mask);
	}
	LL_GPIO_SetPinPull(gpio, mask, pull);
	LL_GPIO_SetPinSpeed(gpio, mask, speed);
	LL_GPIO_SetPinOutputType(gpio, mask,
				 open_drain ? LL_GPIO_OUTPUT_OPENDRAIN : LL_GPIO_OUTPUT_PUSHPULL);
	if (pin < 8U) {
		LL_GPIO_SetAFPin_0_7(gpio, mask, af);
	} else {
		LL_GPIO_SetAFPin_8_15(gpio, mask, af);
	}
	LL_GPIO_SetPinMode(gpio, mask, mode);
	irq_unlock(key);
}

#endif /* ZEPHYR_SOC_PUYA_PY32_GPIO_H_ */
