/*
 * Copyright (c) 2018 qianfan Zhao <qianfanguijin@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_SYSCFG_REGISTERS_H_
#define _STM32_SYSCFG_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   stm32f2X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 7: System configuration controller (SYSCFG)
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
