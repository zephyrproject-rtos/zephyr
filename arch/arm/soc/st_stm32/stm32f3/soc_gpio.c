/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F303xB/C/D/E, STM32F303x6/8, STM32F328x8, STM32F358xC,
 *   STM32F398xE advanced ARM ® -based MCUs
 *
 * Chapter 11: General-purpose I/Os (GPIO)
 */

#include <errno.h>

#include <device.h>
#include "soc.h"
#include "soc_registers.h"
#include <gpio.h>
#include <gpio/gpio_stm32.h>


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

int stm32_gpio_configure(u32_t *base_addr, int pin, int conf, int altf)
{
	volatile struct stm32f3x_gpio *gpio =
		(struct stm32f3x_gpio *)(base_addr);
	unsigned int mode, otype, ospeed, pupd;
	unsigned int pin_shift = pin << 1;
	unsigned int afr_bank = pin / 8;
	unsigned int afr_shift = (pin % 8) << 2;
	u32_t scratch;

	mode = (conf >> STM32_MODER_SHIFT) & STM32_MODER_MASK;
	otype = (conf >> STM32_OTYPER_SHIFT) & STM32_OTYPER_MASK;
	ospeed = (conf >> STM32_OSPEEDR_SHIFT) & STM32_OSPEEDR_MASK;
	pupd = (conf >> STM32_PUPDR_SHIFT) & STM32_PUPDR_MASK;

	scratch = gpio->moder & ~(STM32_MODER_MASK << pin_shift);
	gpio->moder = scratch | (mode << pin_shift);

	scratch = gpio->ospeedr & ~(STM32_OSPEEDR_MASK << pin_shift);
	gpio->ospeedr = scratch | (ospeed << pin_shift);

	scratch = gpio->otyper & ~(STM32_OTYPER_MASK << pin);
	gpio->otyper = scratch | (otype << pin);

	scratch = gpio->pupdr & ~(STM32_PUPDR_MASK << pin_shift);
	gpio->pupdr = scratch | (pupd << pin_shift);

	scratch = gpio->afr[afr_bank] & ~(STM32_AFR_MASK << afr_shift);
	gpio->afr[afr_bank] = scratch | (altf << afr_shift);

	return 0;
}

int stm32_gpio_set(u32_t *base, int pin, int value)
{
	struct stm32f3x_gpio *gpio = (struct stm32f3x_gpio *)base;

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
	struct stm32f3x_gpio *gpio = (struct stm32f3x_gpio *)base;

	return (gpio->idr >> pin) & 0x1;
}

int stm32_gpio_enable_int(int port, int pin)
{
	volatile struct stm32f3x_syscfg *syscfg =
		(struct stm32f3x_syscfg *)SYSCFG_BASE;
	volatile union syscfg__exticr *exticr;

	/* Enable System Configuration Controller clock. */
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	struct stm32_pclken pclken = {
		.bus = STM32_CLOCK_BUS_APB2,
		.enr = LL_APB2_GRP1_PERIPH_SYSCFG
	};

	clock_control_on(clk, (clock_control_subsys_t *) &pclken);

	int shift = 0;

	if (pin <= 3) {
		exticr = &syscfg->exticr1;
	} else if (pin <= 7) {
		exticr = &syscfg->exticr2;
	} else if (pin <= 11) {
		exticr = &syscfg->exticr3;
	} else if (pin <= 15) {
		exticr = &syscfg->exticr4;
	} else {
		return -EINVAL;
	}

	shift = 4 * (pin % 4);

	exticr->val &= ~(0xf << shift);
	exticr->val |= port << shift;

	return 0;
}
