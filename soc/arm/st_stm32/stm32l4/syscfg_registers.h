/*
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32L4X_SYSCFG_REGISTERS_H_
#define _STM32L4X_SYSCFG_REGISTERS_H_

#define STM32L4X_SYSCFG_EXTICR_PIN_MASK		7

/* SYSCFG registers */
struct stm32l4x_syscfg {
	u32_t memrmp;
	u32_t cfgr1;
	u32_t exticr1;
	u32_t exticr2;
	u32_t exticr3;
	u32_t exticr4;
	u32_t scsr;
	u32_t cfgr2;
	u32_t swpr;
	u32_t skr;
};

#endif /* _STM32L4X_SYSCFG_REGISTERS_H_ */
