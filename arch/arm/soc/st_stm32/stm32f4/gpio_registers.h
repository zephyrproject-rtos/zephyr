/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F4X_GPIO_REGISTERS_H_
#define _STM32F4X_GPIO_REGISTERS_H_

/**
 * @brief Driver for GPIO of STM32F4X family processor.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 8: General-purpose I/Os (GPIOs)
 */

/* 8.4 GPIO registers - each GPIO port controls 16 pins */
struct stm32f4x_gpio {
	u32_t mode;
	u32_t otype;
	u32_t ospeed;
	u32_t pupdr;
	u32_t idr;
	u32_t odr;
	u32_t bsr;
	u32_t lck;
	u32_t afr[2];
};

union syscfg_exticr {
	u32_t val;
	struct {
		u16_t rsvd__16_31;
		u16_t exti;
	} bit;
};

/* 7.2 SYSCFG registers */
struct stm32f4x_syscfg {
	u32_t memrmp;
	u32_t pmc;
	union syscfg_exticr exticr1;
	union syscfg_exticr exticr2;
	union syscfg_exticr exticr3;
	union syscfg_exticr exticr4;
	u32_t cmpcr;
};

#endif /* _STM32F4X_GPIO_REGISTERS_H_ */
