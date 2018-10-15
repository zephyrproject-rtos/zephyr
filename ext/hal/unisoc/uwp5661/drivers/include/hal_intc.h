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
#define AON_INT_IRQ_REQ_BB_TS  11
#define AON_INT_UART	12

	static inline void uwp_irq_enable(u32_t channel)
	{
		sci_reg_or(REG_IRQ_ENABLE, BIT(channel));
	}

	static inline void uwp_irq_disable(u32_t channel)
	{
		sci_reg_or(REG_IRQ_DISABLE, BIT(channel));
	}

	static inline u32_t uwp_irq_status(u32_t channel)
	{
		u32_t reg;

		reg = sci_read32(REG_IRQ_RAW_STS);

		return (reg >> channel) & 0x1;
	}

	static inline void uwp_irq_trigger_soft(void)
	{
		sci_write32(REG_IRQ_SOFT, BIT(INT_SOFT));
	}

	static inline void uwp_irq_clear_soft(void)
	{
		sci_write32(REG_IRQ_SOFT, ~BIT(INT_SOFT));
	}

	static inline void uwp_fiq_enable(u32_t channel)
	{
		sci_reg_or(REG_FIQ_ENABLE, BIT(channel));
	}

	static inline void uwp_fiq_disable(u32_t channel)
	{
		sci_reg_or(REG_FIQ_DISABLE, BIT(channel));
	}

	static inline u32_t uwp_fiq_status(u32_t channel)
	{
		u32_t reg;

		reg = sci_read32(REG_FIQ_RAW_STS);

		return (reg >> channel) & 0x1;
	}

	static inline void uwp_fiq_trigger_soft(void)
	{
		sci_write32(REG_FIQ_SOFT, BIT(INT_SOFT));
	}

	static inline void uwp_fiq_clear_soft(void)
	{
		sci_write32(REG_FIQ_SOFT, ~BIT(INT_SOFT));
	}

	static inline void uwp_aon_irq_enable(u32_t channel)
	{
		sci_reg_or(REG_AON_IRQ_ENABLE, BIT(channel));
	}

	static inline void uwp_aon_irq_disable(u32_t channel)
	{
		sci_reg_or(REG_AON_IRQ_DISABLE, BIT(channel));
	}

	static inline u32_t uwp_aon_irq_status(u32_t channel)
	{
		u32_t reg;

		reg = sci_read32(REG_AON_IRQ_RAW_STS);

		return (reg >> channel) & 0x1;
	}

	static inline void uwp_aon_irq_trigger_soft(void)
	{
		sci_write32(REG_AON_IRQ_SOFT, BIT(INT_SOFT));
	}

	static inline void uwp_aon_irq_clear_soft(void)
	{
		sci_write32(REG_AON_IRQ_SOFT, ~BIT(INT_SOFT));
	}
#ifdef __cplusplus
}
#endif

#endif
