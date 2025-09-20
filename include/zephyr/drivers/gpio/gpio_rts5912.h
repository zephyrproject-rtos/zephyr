/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_RTS5912_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_RTS5912_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <reg/reg_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

gpio_pin_t gpio_rts5912_get_intr_pin(volatile uint32_t *reg_base);

static ALWAYS_INLINE void gpio_rts5912_set_wakeup_pin(uint32_t pin_num)
{
	volatile uint32_t *gcr =
		(volatile uint32_t *)(((uint32_t *)(DT_REG_ADDR(DT_NODELABEL(gpioa)))) + pin_num);

	*gcr &= ~(GPIO_GCR_MFCTRL_Msk | GPIO_GCR_DIR_Msk);
	*gcr |= BIT(GPIO_GCR_INTCTRL_Pos) | GPIO_GCR_INTSTS_Msk | GPIO_GCR_INTEN_Msk |
		GPIO_GCR_INDETEN_Msk;
}

int gpio_rts5912_get_pin_num(const struct gpio_dt_spec *gpio);

uint32_t *gpio_rts5912_get_port_address(const struct gpio_dt_spec *gpio);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_RTS5912_H_ */
