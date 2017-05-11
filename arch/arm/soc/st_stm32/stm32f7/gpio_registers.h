/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2017 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F7X_GPIO_REGISTERS_H_
#define _STM32F7X_GPIO_REGISTERS_H_

/* 8.4 GPIO registers - each GPIO port controls 16 pins */
struct stm32f7x_gpio {
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
