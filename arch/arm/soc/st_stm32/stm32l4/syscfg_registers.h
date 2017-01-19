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
	uint32_t memrmp;
	uint32_t cfgr1;
	uint32_t exticr1;
	uint32_t exticr2;
	uint32_t exticr3;
	uint32_t exticr4;
	uint32_t scsr;
	uint32_t cfgr2;
	uint32_t swpr;
	uint32_t skr;
};

#endif /* _STM32L4X_SYSCFG_REGISTERS_H_ */
