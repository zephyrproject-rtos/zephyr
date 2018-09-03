/*
 * Copyright (c) 2018 qianfan Zhao <qianfanguijin@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F2X_GPIO_REGISTERS_H_
#define _STM32F2X_GPIO_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   stm32f2X advanced ARM ® -based 32-bit MCUs

 *
 * Chapter 6: General-purpose I/Os (GPIO)
 * Chapter 7: System configuration controller (SYSCFG)
 */

struct stm32f2x_gpio {
	u32_t moder;
	u32_t otyper;
	u32_t ospeedr;
	u32_t pupdr;
	u32_t idr;
	u32_t odr;
	u32_t bsrr;
	u32_t lckr;
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
struct stm32f2x_syscfg {
	u32_t memrmp;
	u32_t pmc;
	union syscfg_exticr exticr1;
	union syscfg_exticr exticr2;
	union syscfg_exticr exticr3;
	union syscfg_exticr exticr4;
	u32_t cmpcr;
};

#endif /* _STM32F2X_GPIO_REGISTERS_H_ */
