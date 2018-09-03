/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32L4x1, STM32L4x2, STM32L431xx STM32L443xx STM32L433xx, STM32L4x5,
 *   STM32l4x6 advanced ARM ® -based 32-bit MCUs
 *
 * General-purpose I/Os (GPIOs)
 */

#include <errno.h>

#include <device.h>
#include "soc.h"
#include "soc_registers.h"
#include <gpio.h>
#include <gpio/gpio_stm32.h>


enum {
	STM32L4X_PIN3			= 3,
	STM32L4X_PIN7			= 7,
	STM32L4X_PIN11			= 11,
	STM32L4X_PIN15			= 15,
};

#define STM32L4X_IDR_PIN_MASK		0x1
#define STM32L4X_AFR_MASK		0xf

/* GPIO registers - each GPIO port controls 16 pins */
struct stm32l4x_gpio {
	u32_t moder;
	u32_t otyper;
	u32_t ospeedr;
	u32_t pupdr;
	u32_t idr;
	u32_t odr;
	u32_t bsrr;
	u32_t lckr;
	u32_t afr[2];
	u32_t brr;
	u32_t ascr; /* Only present on STM32L4x1, STM32L4x5, STM32L4x6 */
};

int stm32_gpio_flags_to_conf(int flags, int *pincfg)
{
	int direction = flags & GPIO_DIR_MASK;
	int pud = flags & GPIO_PUD_MASK;

	if (!pincfg) {
		return -EINVAL;
	}

	if (direction == GPIO_DIR_OUT) {
		*pincfg = STM32_MODER_OUTPUT_MODE;
	} else {
		/* pull-{up,down} maybe? */
		*pincfg = STM32_MODER_INPUT_MODE;

		if (pud == GPIO_PUD_PULL_UP) {
			*pincfg = *pincfg | STM32_PUPDR_PULL_UP;
		} else if (pud == GPIO_PUD_PULL_DOWN) {
			*pincfg = *pincfg | STM32_PUPDR_PULL_DOWN;
		} else {
			/* floating */
			*pincfg = *pincfg | STM32_PUPDR_NO_PULL;
		}
	}

	return 0;
}

int stm32_gpio_configure(u32_t *base_addr, int pin, int pinconf, int afnum)
{
	volatile struct stm32l4x_gpio *gpio =
		(struct stm32l4x_gpio *)(base_addr);
	unsigned int mode, otype, ospeed, pupd;
	unsigned int pin_shift = pin << 1;
	unsigned int afr_bank = pin / 8;
	unsigned int afr_shift = (pin % 8) << 2;
	u32_t scratch;

	mode = (pinconf >> STM32_MODER_SHIFT) & STM32_MODER_MASK;
	otype = (pinconf >> STM32_OTYPER_SHIFT) & STM32_OTYPER_MASK;
	ospeed = (pinconf >> STM32_OSPEEDR_SHIFT) & STM32_OSPEEDR_MASK;
	pupd = (pinconf >> STM32_PUPDR_SHIFT) & STM32_PUPDR_MASK;

	scratch = gpio->moder & ~(STM32_MODER_MASK << pin_shift);
	gpio->moder = scratch | (mode << pin_shift);

	scratch = gpio->ospeedr & ~(STM32_OSPEEDR_MASK << pin_shift);
	gpio->ospeedr = scratch | (ospeed << pin_shift);

	scratch = gpio->otyper & ~(STM32_OTYPER_MASK << pin);
	gpio->otyper = scratch | (otype << pin);

	scratch = gpio->pupdr & ~(STM32_PUPDR_MASK << pin_shift);
	gpio->pupdr = scratch | (pupd << pin_shift);

	scratch = gpio->afr[afr_bank] & ~(STM32_AFR_MASK << afr_shift);
	gpio->afr[afr_bank] = scratch | (afnum << afr_shift);

	return 0;
}

int stm32_gpio_set(u32_t *base, int pin, int value)
{
	struct stm32l4x_gpio *gpio = (struct stm32l4x_gpio *)base;
	int pval = 1 << (pin & 0xf);

	if (value) {
		gpio->odr |= pval;
	} else {
		gpio->odr &= ~pval;
	}

	return 0;
}

int stm32_gpio_get(u32_t *base, int pin)
{
	struct stm32l4x_gpio *gpio = (struct stm32l4x_gpio *)base;

	return (gpio->idr >> pin) & STM32L4X_IDR_PIN_MASK;
}

int stm32_gpio_enable_int(int port, int pin)
{
	struct stm32l4x_syscfg *syscfg = (struct stm32l4x_syscfg *)SYSCFG_BASE;
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	u32_t *reg;

	/* Enable SYSCFG clock */
	struct stm32_pclken pclken = {
		.bus = STM32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG
	};
	clock_control_on(clk, (clock_control_subsys_t *) &pclken);

	if (pin <= STM32L4X_PIN3) {
		reg = &syscfg->exticr1;
	} else if (pin <= STM32L4X_PIN7) {
		reg = &syscfg->exticr2;
	} else if (pin <= STM32L4X_PIN11) {
		reg = &syscfg->exticr3;
	} else if (pin <= STM32L4X_PIN15) {
		reg = &syscfg->exticr4;
	} else {
		return -EINVAL;
	}

	*reg &= ~(STM32L4X_SYSCFG_EXTICR_PIN_MASK << ((pin % 4) * 4));
	*reg |= port << ((pin % 4) * 4);

	return 0; /* Nothing to do here for STM32L4s */
}
