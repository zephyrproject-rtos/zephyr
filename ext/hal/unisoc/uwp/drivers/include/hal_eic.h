/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_EIC_H
#define __HAL_EIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define EIC_DBNC			(0x0)
#define EIC_LATCH			(0x80)
#define EIC_ASYNC			(0xA0)
#define EIC_SYNC			(0xC0)

#define EIC_LATCH_INTEN		(EIC_LATCH + 0X0)
#define EIC_LATCH_INTRAW	(EIC_LATCH + 0X4)
#define EIC_LATCH_INTMSK	(EIC_LATCH + 0X8)
#define EIC_LATCH_INTCLR	(EIC_LATCH + 0XC)
#define EIC_LATCH_INTPOL	(EIC_LATCH + 0X10)
#define EIC_LATCH_INTMODE	(EIC_LATCH + 0X14)

#define EIC_HIGH_LEVEL_TRIGGER	0
#define EIC_LOW_LEVEL_TRIGGER	1

#define EIC_MAX_NUM				(3)

	static u32_t eic_base[3] = {
		BASE_EIC0,
		BASE_EIC1,
		BASE_AON_EIC0,
	};

	static inline void uwp_hal_eic_enable(u32_t num, u32_t channel)
	{
		sci_reg_or(eic_base[num] + EIC_LATCH_INTEN, BIT(channel));
	}

	static inline void uwp_hal_eic_disable(u32_t num, u32_t channel)
	{
		sci_reg_and(eic_base[num] + EIC_LATCH_INTEN, ~BIT(channel));
	}

	static inline void uwp_hal_eic_enable_sleep(u32_t num, u32_t channel)
	{
		sci_reg_and(eic_base[num] + EIC_LATCH_INTMODE, ~BIT(channel));
	}

	static inline void uwp_hal_eic_disable_sleep(u32_t num, u32_t channel)
	{
		sci_reg_or(eic_base[num] + EIC_LATCH_INTMODE, BIT(channel));
	}

	static inline u32_t uwp_hal_eic_status(u32_t num, u32_t channel)
	{
		u32_t reg;

		//reg = sci_read32(REG_EIC0_LATCH_INTRAW);
		reg = sci_read32(eic_base[num] + EIC_LATCH_INTMSK);

		return (reg >> channel) & 0x1;
	}

	static inline void uwp_hal_eic_clear(u32_t num, u32_t channel)
	{
		sci_write32(eic_base[num] + EIC_LATCH_INTCLR, BIT(channel));
	}

	static inline void uwp_hal_eic_set_trigger(u32_t num,
			u32_t channel, u32_t type)
	{
		u32_t reg;

		reg = sci_read32(eic_base[num] + EIC_LATCH_INTPOL);

		if (type == EIC_HIGH_LEVEL_TRIGGER)
			reg &= ~BIT(channel);
		else
			reg |= BIT(channel);

		sci_write32(eic_base[num] + EIC_LATCH_INTPOL, reg);
	}
#ifdef __cplusplus
}
#endif

#endif
