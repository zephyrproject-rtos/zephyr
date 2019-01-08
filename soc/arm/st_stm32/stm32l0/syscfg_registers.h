/*
 * Copyright (c) 2018 Endre Karlson <endre.karlson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_SYSCFG_REGISTERS_H_
#define _STM32_SYSCFG_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32L0X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 10: System configuration controller (SYSCFG)
 */

#include "../common/soc_syscfg_common.h"

struct stm32_syscfg {
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

#endif /* _STM32_SYSCFG_REGISTERS_H_ */
