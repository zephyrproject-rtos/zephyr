/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_INTC_H
#define __HAL_INTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define REG_IRQ_MASK_STS	(BASE_INTC + 0x0)
#define REG_IRQ_RAW_STS		(BASE_INTC + 0x4)
#define REG_IRQ_ENABLE		(BASE_INTC + 0x8)
#define REG_IRQ_DISABLE		(BASE_INTC + 0xC)
#define REG_IRQ_SOFT		(BASE_INTC + 0x10)

#define REG_FIQ_MASK_STS	(BASE_INTC + 0x20)
#define REG_FIQ_RAW_STS		(BASE_INTC + 0x24)
#define REG_FIQ_ENABLE		(BASE_INTC + 0x28)
#define REG_FIQ_DISABLE		(BASE_INTC + 0x2C)
#define REG_FIQ_SOFT		(BASE_INTC + 0x30)

#define REG_AON_IRQ_MASK_STS	(BASE_AON_INTC + 0x0)
#define REG_AON_IRQ_RAW_STS		(BASE_AON_INTC + 0x4)
#define REG_AON_IRQ_ENABLE		(BASE_AON_INTC + 0x8)
#define REG_AON_IRQ_DISABLE		(BASE_AON_INTC + 0xC)
#define REG_AON_IRQ_SOFT		(BASE_AON_INTC + 0x10)

#define INTC_MAX_CHANNEL    (32)
#define INT_EIC         0
#define INT_SOFT		1

#define AON_INT_GPIO0	10
#define AON_INT_GPIO1	9
#define AON_INT_GPIO2	8
#define AON_INT_IRQ_REQ_BB_TS  11
#define AON_INT_UART	12

	struct uwp_intc {
		u32_t mask_sts;
		u32_t raw_sts;
		u32_t enable;
		u32_t disable;
		u32_t irq_soft;
		u32_t test_src;
		u32_t test_sel;
		u32_t reserved;
	};

	static inline void uwp_intc_enable(volatile struct uwp_intc *intc, u32_t ch)
	{
		intc->enable |= BIT(ch);
	}

	static inline void uwp_intc_disable(volatile struct uwp_intc *intc, u32_t ch)
	{
		intc->disable |= BIT(ch);
	}

	static inline u32_t uwp_intc_status(volatile struct uwp_intc *intc)
	{
		return intc->mask_sts;
	}

	static inline void uwp_intc_trigger_soft(volatile struct uwp_intc *intc)
	{
		intc->irq_soft |= BIT(INT_SOFT);
	}

	static inline void uwp_intc_clear_soft(volatile struct uwp_intc *intc)
	{
		intc->irq_soft &= ~BIT(INT_SOFT);
	}

#ifdef __cplusplus
}
#endif

#endif
