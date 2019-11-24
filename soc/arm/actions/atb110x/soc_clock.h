/*
 * Copyright (c) 2019 Actions (Zhuhai) Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Peripheral clock configuration macros for ATB110X
 */

#ifndef _ACTIONS_SOC_CLOCK_H_
#define _ACTIONS_SOC_CLOCK_H_

#include <stdbool.h>
#include "soc_regs.h"

#define ACT_32M_CTL			(CMU_DIGITAL_REG_BASE + 0x0004)
#define ACT_3M_CTL			(CMU_DIGITAL_REG_BASE + 0x0008)
#define CMU_SYSCLK			(CMU_DIGITAL_REG_BASE + 0x000C)

#define ACT_32M_CTL_XTAL32M_EN		0

#define ACT_3M_CTL_RC3M_OK		5
#define ACT_3M_CTL_RSEL_E		4
#define ACT_3M_CTL_RSEL_SHIFT		1
#define ACT_3M_CTL_RSEL_MASK		(0xf << 1)
#define ACT_3M_CTL_RC3MEN		0

#define CMU_SYSCLK_BLE_HCLK_DIV_E	17
#define CMU_SYSCLK_BLE_HCLK_DIV_SHIFT	16
#define CMU_SYSCLK_BLE_HCLK_DIV_MASK	(0x3 << 16)
#define CMU_SYSCLK_BLE_HCLK_SEL		15
#define CMU_SYSCLK_HOSC_A		13
#define CMU_SYSCLK_LOSC_A		12
#define CMU_SYSCLK_APB_DIV_E		11
#define CMU_SYSCLK_APB_DIV_SHIFT	10
#define CMU_SYSCLK_APB_DIV_MASK		(0x3 << 10)
#define CMU_SYSCLK_AHB_DIV		9
#define CMU_SYSCLK_CPU_COREPLL		8
#define CMU_SYSCLK_CPU_32M		7
#define CMU_SYSCLK_CPU_32K		6
#define CMU_SYSCLK_CPU_3M		5
#define CMU_SYSCLK_SLEEP_HFCLK_SEL	4
#define CMU_SYSCLK_PCLKG		2
#define CMU_SYSCLK_CPU_CLK_SEL_E	1
#define CMU_SYSCLK_CPU_CLK_SEL_SHIFT	0
#define CMU_SYSCLK_CPU_CLK_SEL_MASK	(0x3 << 0)

void acts_request_rc_3M(bool enable);

#endif /* _ACTIONS_SOC_CLOCK_H_ */
