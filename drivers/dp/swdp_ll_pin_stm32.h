/*
 * Copyright 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_ll_gpio.h>

#define CPU_CLOCK CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define FAST_BITBANG_HW_SUPPORT 1

static ALWAYS_INLINE void swdp_ll_pin_input(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	LL_GPIO_SetPinMode(gpio, BIT(pin), LL_GPIO_MODE_INPUT);
}

static ALWAYS_INLINE void swdp_ll_pin_output(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	LL_GPIO_SetPinMode(gpio, BIT(pin), LL_GPIO_MODE_OUTPUT);
}

static ALWAYS_INLINE void swdp_ll_pin_set(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;
	uint32_t val;

	val = LL_GPIO_ReadOutputPort(gpio);
	val |= BIT(pin);
	LL_GPIO_WriteOutputPort(gpio, val);
}

static ALWAYS_INLINE void swdp_ll_pin_clr(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;
	uint32_t val;

	val = LL_GPIO_ReadOutputPort(gpio);
	val &= ~BIT(pin);
	LL_GPIO_WriteOutputPort(gpio, val);
}

static ALWAYS_INLINE uint32_t swdp_ll_pin_get(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	return (LL_GPIO_ReadInputPort(gpio) >> pin) & 1;
}
