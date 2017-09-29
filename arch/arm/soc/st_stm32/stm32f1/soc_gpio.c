/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 9: General-purpose and alternate-function I/Os
 *            (GPIOs and AFIOs)
 */

#include <errno.h>

#include <device.h>
#include "soc.h"
#include "soc_registers.h"
#include <gpio.h>
#include <gpio/gpio_stm32.h>
#include <pinmux/stm32/pinmux_stm32.h>


int stm32_gpio_flags_to_conf(int flags, int *pincfg)
{
	int direction = flags & GPIO_DIR_MASK;

	if (!pincfg) {
		return -EINVAL;
	}

	if (direction == GPIO_DIR_OUT) {
		/* Pin is configured as an output */
		*pincfg = (STM32_MODE_OUTPUT | STM32_CNF_GP_OUTPUT |
			   STM32_CNF_PUSH_PULL);
	} else {
		/* Pin is configured as an input */
		int pud = flags & GPIO_PUD_MASK;

		/* pull-{up,down} maybe? */
		if (pud == GPIO_PUD_PULL_UP) {
			*pincfg = (STM32_MODE_INPUT | STM32_CNF_IN_PUPD |
				   STM32_PUPD_PULL_UP);
		} else if (pud == GPIO_PUD_PULL_DOWN) {
			*pincfg = (STM32_MODE_INPUT | STM32_CNF_IN_PUPD |
				   STM32_PUPD_PULL_DOWN);
		} else {
			/* floating */
			*pincfg = (STM32_MODE_INPUT | STM32_CNF_IN_FLOAT |
				   STM32_PUPD_NO_PULL);
		}
	}

	return 0;
}

int stm32_gpio_configure(u32_t *base_addr, int pin, int conf, int altf)
{
	volatile struct stm32f10x_gpio *gpio =
		(struct stm32f10x_gpio *)(base_addr);
	int cnf, mode, mode_io;
	int crpin = pin;

	/* pins are configured in CRL (0-7) and CRH (8-15)
	 * registers
	 */
	volatile u32_t *reg = &gpio->crl;

	ARG_UNUSED(altf);

	if (crpin > 7) {
		reg = &gpio->crh;
		crpin -= 8;
	}

	/* each port is configured by 2 registers:
	 * CNFy[1:0]: Port x configuration bits
	 * MODEy[1:0]: Port x mode bits
	 *
	 * memory layout is repeated for every port:
	 *   |  CNF  |  MODE |
	 *   | [0:1] | [0:1] |
	 */

	mode_io = (conf >> STM32_MODE_INOUT_SHIFT) & STM32_MODE_INOUT_MASK;

	if (mode_io == STM32_MODE_INPUT) {
		int in_pudpd = conf & (STM32_PUPD_MASK << STM32_PUPD_SHIFT);

		/* Pin configured in input mode */
		/* Mode: 00 */
		mode = mode_io;
		/* Configuration values: */
		/* 00: Analog mode */
		/* 01: Floating input */
		/* 10: Pull-up/Pull-Down */
		cnf = (conf >> STM32_CNF_IN_SHIFT) & STM32_CNF_IN_MASK;

		if (in_pudpd == STM32_PUPD_PULL_UP) {
			/* enable pull up */
			gpio->odr |= 1 << pin;
		} else if (in_pudpd == STM32_PUPD_PULL_DOWN) {
			/* or pull down */
			gpio->odr &= ~(1 << pin);
		}
	} else {
		/* Pin configured in output mode */
		int mode_speed = ((conf >> STM32_MODE_OSPEED_SHIFT) & \
				  STM32_MODE_OSPEED_MASK);
		/* Mode output possible values */
		/* 01: Max speed 10MHz (default value) */
		/* 10: Max speed 2MHz */
		/* 11: Max speed 50MHz */
		mode = mode_speed + mode_io;
		/* Configuration possible values */
		/* x0: Push-pull */
		/* x1: Open-drain */
		/* 0x: General Purpose Output */
		/* 1x: Alternate Function Output */
		cnf = ((conf >> STM32_CNF_OUT_0_SHIFT) & STM32_CNF_OUT_0_MASK) |
		      (((conf >> STM32_CNF_OUT_1_SHIFT) & STM32_CNF_OUT_1_MASK)
		       << 1);
	}

	/* clear bits */
	*reg &= ~(0xf << (crpin * 4));
	/* set bits */
	*reg |= (cnf << (crpin * 4 + 2) | mode << (crpin * 4));

	return 0;
}

int stm32_gpio_set(u32_t *base, int pin, int value)
{
	struct stm32f10x_gpio *gpio = (struct stm32f10x_gpio *)base;

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
	struct stm32f10x_gpio *gpio = (struct stm32f10x_gpio *)base;

	return (gpio->idr >> pin) & 0x1;
}

int stm32_gpio_enable_int(int port, int pin)
{
	volatile struct stm32f10x_afio *afio =
		(struct stm32f10x_afio *)AFIO_BASE;
	volatile union __afio_exticr *exticr;
	int shift = 0;

	if (pin <= 3) {
		exticr = &afio->exticr1;
	} else if (pin <= 7) {
		exticr = &afio->exticr2;
	} else if (pin <= 11) {
		exticr = &afio->exticr3;
	} else if (pin <= 15) {
		exticr = &afio->exticr4;
	} else {
		return -EINVAL;
	}

	shift = 4 * (pin % 4);

	exticr->val &= ~(0xf << shift);
	exticr->val |= port << shift;

	return 0;
}
