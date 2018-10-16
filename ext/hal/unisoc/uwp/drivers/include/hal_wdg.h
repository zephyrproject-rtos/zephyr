/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_WDG_H
#define __HAL_WDG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define WDG_NEWMODE

#define REG_WDG_LOAD_LOW		(BASE_WDG + 0x0)
#define REG_WDG_LOAD_HIGH		(BASE_WDG + 0x4)
#define REG_WDG_CTRL			(BASE_WDG + 0x8)
#define REG_WDG_INT_CLR			(BASE_WDG + 0xC)
#define REG_WDG_INT_RAW			(BASE_WDG + 0x10)
#define REG_WDG_INT_MASK		(BASE_WDG + 0x14)
#define REG_WDG_CNT_LOW			(BASE_WDG + 0x18)
#define REG_WDG_CNT_HIGH		(BASE_WDG + 0x1C)
#define REG_WDG_LOCK			(BASE_WDG + 0x20)
#define REG_WDG_CNT_RD_LOW		(BASE_WDG + 0x24)
#define REG_WDG_CNT_RD_HIGH		(BASE_WDG + 0x28)
#define REG_WDG_IRQ_VAL_LOW		(BASE_WDG + 0x2C)
#define REG_WDG_IRQ_VAL_HIGH	(BASE_WDG + 0x30)

#define WDG_UNLOCK_KEY			0xE551
#define WDG_LOCK_KEY			0x551

#define WDG_MODE_RESET 1
#define WDG_MODE_IRQ   2
#define WDG_MODE_MIX   3

#define WDG_RST_EN  BIT(3)
#define WDG_NEW     BIT(2)
#define WDG_RUN     BIT(1)
#define WDG_IRQ_EN  BIT(0)

#define WDG_RST_CLR		BIT(3)
#define WDG_INT_CLR_BIT BIT(0)

#define WDG_LD_BUSY		BIT(4)
#define WDG_RST_RAW		BIT(3)
#define WDG_INT_RAW_BIT BIT(0)

#define WDG_IRQ_MSK_BIT BIT(0)

	struct uwp_wdg {
		u32_t load_low;
		u32_t load_high;
		u32_t ctrl;
		u32_t int_clr;
		u32_t int_raw;
		u32_t int_mask;
		u32_t cnt_low;
		u32_t cnt_high;
		u32_t lock;
		u32_t cnt_rd_low;
		u32_t cnt_rd_high;
		u32_t irq_val_low;
		u32_t irq_val_high;
	};

	static inline void uwp_wdg_lock(void)
	{
		sci_write32(REG_WDG_LOCK, WDG_LOCK_KEY);
	}

	static inline void uwp_wdg_unlock(void)
	{
		sci_write32(REG_WDG_LOCK, WDG_UNLOCK_KEY);
	}

	static inline void uwp_wdg_set_mode(u32_t mode)
	{
		u32_t mode_bitmap;

		switch (mode){
			case WDG_MODE_RESET:
				mode_bitmap = WDG_RST_EN;
				break;
			case WDG_MODE_IRQ:
				mode_bitmap = WDG_IRQ_EN;
				break;
			case WDG_MODE_MIX:
				mode_bitmap = WDG_IRQ_EN|WDG_RST_EN;
				break;
			default :
				mode_bitmap = 0;
		}

		uwp_wdg_unlock();
		//clear mode bit
		sci_reg_and(REG_WDG_CTRL, ~(WDG_RST_EN|WDG_IRQ_EN));

		mode_bitmap |=WDG_NEW;

		//set mode bit
		sci_reg_or(REG_WDG_CTRL, mode_bitmap);

		uwp_wdg_lock();
	}

	static inline void uwp_wdg_load(u32_t value)
	{
#ifndef WDG_NEWMODE
		u32_t cnt = 0;
#endif
		value = value * 32768 / 1000;
		uwp_wdg_unlock();

#ifndef WDG_NEWMODE
		while((sci_read32(REG_WDG_INT_RAW) &WDG_LD_BUSY) && cnt++ < 200);
#endif
		sci_write32(REG_WDG_LOAD_HIGH, (value>>16)&0xffff);
		sci_write32(REG_WDG_LOAD_LOW, value&0xffff);

		uwp_wdg_lock();
	}

	static inline void uwp_wdg_load_irq(u32_t value)
	{
#ifndef WDG_NEWMODE
		u32_t cnt = 0;
#endif
		uwp_wdg_unlock();
		value = value * 32768 / 1000;

#ifndef WDG_NEWMODE
		while((sci_read32(REG_WDG_INT_RAW) &WDG_LD_BUSY) && cnt++ < 200);
#endif
		sci_write32(REG_WDG_IRQ_VAL_HIGH, (value>>16)&0xffff);
		sci_write32(REG_WDG_IRQ_VAL_LOW, value&0xffff);

		uwp_wdg_lock();
	}

	static inline void uwp_wdg_enable(void)
	{
		uwp_wdg_unlock();

		sci_reg_or(REG_WDG_CTRL, WDG_RUN);

		uwp_wdg_lock();
	}

	static inline void uwp_wdg_disable(void)
	{
		uwp_wdg_unlock();

		sci_reg_and(REG_WDG_CTRL, ~WDG_RUN);

		uwp_wdg_lock();
	}

	static inline void uwp_wdg_int_clear(void)
	{
		uwp_wdg_unlock();

		sci_reg_or(REG_WDG_INT_CLR, WDG_INT_CLR_BIT);

		uwp_wdg_lock();
	}

	static inline u32_t uwp_wdg_get_counter(void)
	{
		u32_t value = 0;

		uwp_wdg_unlock();

		value = (sci_read32(REG_WDG_CNT_RD_LOW)&0xffff) |
			((sci_read32(REG_WDG_CNT_RD_HIGH)&0xffff)<<16);

		uwp_wdg_lock();

		return value;
	}

#ifdef __cplusplus
}
#endif

#endif
