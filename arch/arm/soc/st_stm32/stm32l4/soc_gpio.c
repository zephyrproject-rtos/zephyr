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
#include "soc_pinmux.h"
#include <gpio.h>
#include <gpio/gpio_stm32.h>

enum {
	STM32L4X_MODER_INPUT_MODE	= 0x0,
	STM32L4X_MODER_OUTPUT_MODE	= 0x1,
	STM32L4X_MODER_ALT_MODE		= 0x2,
	STM32L4X_MODER_ANALOG_MODE	= 0x3,
	STM32L4X_MODER_MASK		= 0x3,
};

enum {
	STM32L4X_OTYPER_PUSH_PULL	= 0x0,
	STM32L4X_OTYPER_OPEN_DRAIN	= 0x1,
	STM32L4X_OTYPER_MASK		= 0x1,
};

enum {
	STM32L4X_PUPDR_NO_PULL		= 0x0,
	STM32L4X_PUPDR_PULL_UP		= 0x1,
	STM32L4X_PUPDR_PULL_DOWN	= 0x2,
	STM32L4X_PUPDR_MASK		= 0x3,
};

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
	uint32_t moder;
	uint32_t otyper;
	uint32_t ospeedr;
	uint32_t pupdr;
	uint32_t idr;
	uint32_t odr;
	uint32_t bsrr;
	uint32_t lckr;
	uint32_t afr[2];
	uint32_t brr;
	uint32_t ascr; /* Only present on STM32L4x1, STM32L4x5, STM32L4x6 */
};

/**
 * @brief map pin function to MODE register value
 */
static uint32_t func_to_mode(int conf, unsigned int afnum)
{
	/* If an alternate function is specified */
	if (afnum) {
		return STM32L4X_MODER_ALT_MODE;
	}

	switch (conf) {
	case STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
	case STM32L4X_PIN_CONFIG_BIAS_PULL_UP:
	case STM32L4X_PIN_CONFIG_BIAS_PULL_DOWN:
		return STM32L4X_MODER_INPUT_MODE;
	case STM32L4X_PIN_CONFIG_ANALOG:
		return STM32L4X_MODER_ANALOG_MODE;
	default:
		return STM32L4X_MODER_OUTPUT_MODE;
	}

	return STM32L4X_MODER_INPUT_MODE;
}

static uint32_t func_to_otype(int conf)
{
	switch (conf) {
	case STM32L4X_PIN_CONFIG_OPEN_DRAIN:
	case STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_UP:
	case STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_DOWN:
		return STM32L4X_OTYPER_OPEN_DRAIN;
	default:
		return STM32L4X_OTYPER_PUSH_PULL;
	}

	return STM32L4X_OTYPER_PUSH_PULL;
}

static uint32_t func_to_pupd(int conf)
{
	switch (conf) {
	case STM32L4X_PIN_CONFIG_ANALOG:
	case STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
	case STM32L4X_PIN_CONFIG_PUSH_PULL:
	case STM32L4X_PIN_CONFIG_OPEN_DRAIN:
		return STM32L4X_PUPDR_NO_PULL;
	case STM32L4X_PIN_CONFIG_BIAS_PULL_UP:
	case STM32L4X_PIN_CONFIG_PUSH_PULL_PULL_UP:
	case STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_UP:
		return STM32L4X_PUPDR_PULL_UP;
	case STM32L4X_PIN_CONFIG_BIAS_PULL_DOWN:
	case STM32L4X_PIN_CONFIG_PUSH_PULL_PULL_DOWN:
	case STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_DOWN:
		return STM32L4X_PUPDR_PULL_DOWN;
	}

	return STM32L4X_PUPDR_NO_PULL;
}

int stm32_gpio_flags_to_conf(int flags, int *pincfg)
{
	int direction = flags & GPIO_DIR_MASK;

	if (!pincfg) {
		return -EINVAL;
	}

	if (direction == GPIO_DIR_OUT) {
		*pincfg = STM32L4X_PIN_CONFIG_PUSH_PULL;
	} else if (direction == GPIO_DIR_IN) {
		int pud = flags & GPIO_PUD_MASK;

		/* pull-{up,down} maybe? */
		if (pud == GPIO_PUD_PULL_UP) {
			*pincfg = STM32L4X_PIN_CONFIG_BIAS_PULL_UP;
		} else if (pud == GPIO_PUD_PULL_DOWN) {
			*pincfg = STM32L4X_PIN_CONFIG_BIAS_PULL_DOWN;
		} else {
			/* floating */
			*pincfg = STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int stm32_gpio_configure(uint32_t *base_addr, int pin, int pinconf, int afnum)
{
	volatile struct stm32l4x_gpio *gpio =
		(struct stm32l4x_gpio *)(base_addr);
	unsigned int mode, otype, pupd;
	unsigned int pin_shift = pin << 1;
	unsigned int afr_bank = pin / 8;
	unsigned int afr_shift = (pin % 8) << 2;
	uint32_t scratch;

	mode = func_to_mode(pinconf, afnum);
	otype = func_to_otype(pinconf);
	pupd = func_to_pupd(pinconf);

	scratch = gpio->moder & ~(STM32L4X_MODER_MASK << pin_shift);
	gpio->moder = scratch | (mode << pin_shift);

	scratch = gpio->otyper & ~(STM32L4X_OTYPER_MASK << pin);
	gpio->otyper = scratch | (otype << pin);

	scratch = gpio->pupdr & ~(STM32L4X_PUPDR_MASK << pin_shift);
	gpio->pupdr = scratch | (pupd << pin_shift);

	scratch = gpio->afr[afr_bank] & ~(STM32L4X_AFR_MASK << afr_shift);
	gpio->afr[afr_bank] = scratch | (afnum << afr_shift);

	return 0;
}

int stm32_gpio_set(uint32_t *base, int pin, int value)
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

int stm32_gpio_get(uint32_t *base, int pin)
{
	struct stm32l4x_gpio *gpio = (struct stm32l4x_gpio *)base;

	return (gpio->idr >> pin) & STM32L4X_IDR_PIN_MASK;
}

int stm32_gpio_enable_int(int port, int pin)
{
	struct stm32l4x_syscfg *syscfg = (struct stm32l4x_syscfg *)SYSCFG_BASE;
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	uint32_t *reg;

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

	*reg &= STM32L4X_SYSCFG_EXTICR_PIN_MASK << ((pin % 4) * 4);
	*reg |= port << ((pin % 4) * 4);

	return 0; /* Nothing to do here for STM32L4s */
}
