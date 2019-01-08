/*
 * Copyright (c) 2018 Yurii Hamann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_SYSCFG_REGISTERS_H_
#define _STM32_SYSCFG_REGISTERS_H_

/**
 * @brief Driver for GPIO of STM32F7X family processor.
 *
 * Based on reference manual:
 *   RM0385 Reference manual STM32F75xxx and STM32F74xxx
 *   advanced ARM(r)-based 32-bit MCUs
 */

#include "../common/soc_syscfg_common.h"

/* 7.2 SYSCFG registers */
struct stm32_syscfg {
	u32_t memrmp;
	u32_t pmc;
	union syscfg_exticr exticr1;
	union syscfg_exticr exticr2;
	union syscfg_exticr exticr3;
	union syscfg_exticr exticr4;
	u32_t cmpcr;
};

#endif /* _STM32_SYSCFG_REGISTERS_H_ */
