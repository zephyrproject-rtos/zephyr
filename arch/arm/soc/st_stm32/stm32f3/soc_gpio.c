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

/**
 * @brief map pin function to MODE register value
 */
static uint32_t func_to_mode(int func)
{
	switch (func) {
	case STM32F3X_PIN_CONFIG_ANALOG:
		return 0x3;
	case STM32F3X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE:
	case STM32F3X_PIN_CONFIG_BIAS_PULL_UP:
	case STM32F3X_PIN_CONFIG_BIAS_PULL_DOWN:
		return 0x0;
	case STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN:
	case STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL:
	case STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PU:
	case STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PD:
	case STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PU:
	case STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PD:
		return 0x1;
	case STM32F3X_PIN_CONFIG_AF:
		return 0x2;
	}
	return 0;
}

int stm32_gpio_flags_to_conf(int flags, int *pincfg)
{
	int direction = flags & GPIO_DIR_MASK;

	if (!pincfg) {
		return -EINVAL;
	}

	int pud = flags & GPIO_PUD_MASK;

	if (direction == GPIO_DIR_OUT) {
		int type = flags & GPIO_DS_HIGH_MASK;

		if (type == GPIO_DS_DISCONNECT_HIGH) {
			*pincfg = STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN;
			if (pud == GPIO_PUD_PULL_UP) {
				*pincfg =
					STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PU;
			} else if (pud == GPIO_PUD_PULL_DOWN) {
				*pincfg =
					STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PD;
			}
		} else {
			*pincfg = STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL;
			if (pud == GPIO_PUD_PULL_UP) {
				*pincfg =
					STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PU;
			} else if (pud == GPIO_PUD_PULL_DOWN) {
				*pincfg =
					STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PD;
			}
		}
	} else if (direction == GPIO_DIR_IN) {
		if (pud == GPIO_PUD_PULL_UP) {
			*pincfg = STM32F3X_PIN_CONFIG_BIAS_PULL_UP;
		} else if (pud == GPIO_PUD_PULL_DOWN) {
			*pincfg = STM32F3X_PIN_CONFIG_BIAS_PULL_DOWN;
		} else {
			/* floating */
			*pincfg = STM32F3X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int stm32_gpio_configure(uint32_t *base_addr, int pin, int conf, int altf)
{
	volatile struct stm32f3x_gpio *gpio =
		(struct stm32f3x_gpio *)(base_addr);
	int mode, cmode;

	cmode = STM32_MODE(conf);
	mode = func_to_mode(cmode);

	/* clear bits */
	gpio->moder &= ~(0x3 << (pin * 2));
	/* set bits */
	gpio->moder |= (mode << (pin * 2));

	if (cmode == STM32F3X_PIN_CONFIG_AF) {
		/* alternate function setup */
		int af = STM32_AF(conf);
		volatile uint32_t *afr = &gpio->afrl;
		int crpin = pin;

		if (crpin > 7) {
			afr = &gpio->afrh;
			crpin -= 7;
		}

		/* clear AF bits */
		*afr &= ~(0xf << (crpin * 4));
		/* set AF */
		*afr |= (af << (crpin * 4));
	} else if (cmode == STM32F3X_PIN_CONFIG_ANALOG) {
		gpio->pupdr &= ~(0x3 << (pin * 2));
	} else {
		/* clear typer */
		gpio->otyper &= ~(1 << pin);

		if (cmode == STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN ||
		    cmode == STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PU ||
		    cmode == STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PD) {
			/* configure pin as output open-drain */
			gpio->otyper |= 1 << pin;
		}

		/* configure pin as floating by clearing pupd flags */
		gpio->pupdr &= ~(0x3 << (pin * 2));

		if (cmode == STM32F3X_PIN_CONFIG_BIAS_PULL_UP ||
		    cmode == STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PU ||
		    cmode == STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PU) {
			/* enable pull up */
			gpio->pupdr |= (1 << (pin * 2));
		} else if (cmode == STM32F3X_PIN_CONFIG_BIAS_PULL_DOWN ||
			   cmode == STM32F3X_PIN_CONFIG_DRIVE_PUSH_PULL_PD ||
			   cmode == STM32F3X_PIN_CONFIG_DRIVE_OPEN_DRAIN_PD) {
			/* or pull down */
			gpio->pupdr |= (2 << (pin * 2));
		}
	}

	return 0;
}

int stm32_gpio_set(uint32_t *base, int pin, int value)
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

int stm32_gpio_get(uint32_t *base, int pin)
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

	clock_control_on(clk, UINT_TO_POINTER(STM32F3X_CLOCK_SUBSYS_SYSCFG));

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
