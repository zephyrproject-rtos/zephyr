/*
 * Copyright (c) 2018 Endre Karlson <endre.karlson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32L0X_GPIO_REGISTERS_H_
#define _STM32L0X_GPIO_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32L0X advanced ARM ® -based 32-bit MCUs

 *
 * Chapter 9: General-purpose I/Os (GPIO)
 * Chapter 10: System configuration controller (SYSCFG)
 */

struct stm32l0x_gpio {
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
		u16_t exti;
		u16_t rsvd__16_31;
	} bit;
};

struct stm32l0x_syscfg {
	u32_t cfgr1;
	u32_t cfgr2;
	union syscfg_exticr exticr1;
	union syscfg_exticr exticr2;
	union syscfg_exticr exticr3;
	union syscfg_exticr exticr4;
	u32_t comp1_ctrl;
	u32_t comp2_ctrl;
	u32_t cfgr3;
};

#endif /* _STM32L0X_GPIO_REGISTERS_H_ */
