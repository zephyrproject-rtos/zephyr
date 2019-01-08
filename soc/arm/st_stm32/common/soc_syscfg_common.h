/*
 * Copyright (c) 2018 Song Qiang <songqiang1304521@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_SYSCFG_COMMON_H
#define __SOC_SYSCFG_COMMON_H

union syscfg_exticr {
	u32_t val;
	struct {
		u16_t exti;
		u16_t rsvd__16_31;
	} bit;
};

#endif /* __STM32_SYSCFG_COMMON_H */
