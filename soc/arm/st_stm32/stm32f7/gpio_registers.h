/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F7X_GPIO_REGISTERS_H_
#define _STM32F7X_GPIO_REGISTERS_H_

/**
 * @brief Driver for GPIO of STM32F7X family processor.
 *
 * Based on reference manual:
 *   RM0385 Reference manual STM32F75xxx and STM32F74xxx
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 6: General-purpose I/Os (GPIOs)
 */

/* 6.4 GPIO registers - each GPIO port controls 16 pins */
struct stm32f7x_gpio {
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
};

union syscfg_exticr {
	u32_t val;
	struct {
		u16_t rsvd__16_31;
		u16_t exti;
	} bit;
};

/* 7.2 SYSCFG registers */
struct stm32f7x_syscfg {
	u32_t memrmp;
	u32_t pmc;
	union syscfg_exticr exticr1;
	union syscfg_exticr exticr2;
	union syscfg_exticr exticr3;
	union syscfg_exticr exticr4;
	u32_t cmpcr;
};

#endif /* _STM32F7X_GPIO_REGISTERS_H_ */
