/*
 * Copyright 2025 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stm32_ll_gpio.h>
#include "stm32_hsem.h"

#define CPU_CLOCK CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define FAST_BITBANG_HW_SUPPORT 1

static ALWAYS_INLINE void swdp_ll_pin_input(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	z_stm32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	LL_GPIO_SetPinMode(gpio, BIT(pin), LL_GPIO_MODE_INPUT);

	z_stm32_hsem_unlock(CFG_HW_GPIO_SEMID);
}

static ALWAYS_INLINE void swdp_ll_pin_output(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	z_stm32_hsem_lock(CFG_HW_GPIO_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	LL_GPIO_SetPinMode(gpio, BIT(pin), LL_GPIO_MODE_OUTPUT);

	z_stm32_hsem_unlock(CFG_HW_GPIO_SEMID);
}

static ALWAYS_INLINE void swdp_ll_pin_set(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	LL_GPIO_SetOutputPin(gpio, BIT(pin));
}

static ALWAYS_INLINE void swdp_ll_pin_clr(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	LL_GPIO_ResetOutputPin(gpio, BIT(pin));
}

static ALWAYS_INLINE uint32_t swdp_ll_pin_get(void *const base, uint8_t pin)
{
	GPIO_TypeDef *gpio = base;

	return (LL_GPIO_ReadInputPort(gpio) >> pin) & 1;
}
