/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F3X_GPIO_REGISTERS_H_
#define _STM32F3X_GPIO_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F303xB/C/D/E, STM32F303x6/8, STM32F328x8, STM32F358xC,
 *   STM32F398xE advanced ARM(r)-based MCUs
 *
 * Chapter 11: General-purpose I/Os
 */

struct stm32f3x_gpio {
	u32_t moder;
	u32_t otyper;
	u32_t ospeedr;
	u32_t pupdr;
	u32_t idr;
	u32_t odr;
	u32_t bsrr;
	u32_t lckr;
	u32_t afr[2];
	u32_t brr;
};

union syscfg_cfgr1 {
	u32_t val;
	struct {
		u32_t mem_mode :2 __packed;
		u32_t rsvd__2_5 :4 __packed;
		u32_t tim1_itr3_rmo :1 __packed;
		u32_t dac_trig_rmp :1 __packed;
		u32_t rsvd__8_10 :3 __packed;
		u32_t tim16_dma_rmp :1 __packed;
		u32_t tim17_dma_rmp :1 __packed;
		u32_t tim16_dac1_dma_rmp :1 __packed;
		u32_t tim17_dac2_dma_rmp :1 __packed;
		u32_t dac2_ch1_dma_rmp :1 __packed;
		u32_t i2c_pb6_fmp :1 __packed;
		u32_t i2c_pb7_fmp :1 __packed;
		u32_t i2c_pb8_fmp :1 __packed;
		u32_t i2c_pb9_fmp :1 __packed;
		u32_t i2c1_fmp :1 __packed;
		u32_t rsvd__21 :1 __packed;
		u32_t encoder_mode :2 __packed;
		u32_t rsvd__24_25 :2 __packed;
		u32_t fpu_ie :6 __packed;
	} bit;
};

union syscfg_rcr {
	u32_t val;
	struct {
		u32_t page0_wp :1 __packed;
		u32_t page1_wp :1 __packed;
		u32_t page2_wp :1 __packed;
		u32_t page3_wp :1 __packed;
		u32_t rsvd__4_31 :28 __packed;
	} bit;
};

union syscfg__exticr {
	u32_t val;
	struct {
		u16_t exti;
		u16_t rsvd__16_31;
	} bit;
};

struct stm32f3x_syscfg {
	union syscfg_cfgr1 cfgr1;
	union syscfg_rcr rcr;
	union syscfg__exticr exticr1;
	union syscfg__exticr exticr2;
	union syscfg__exticr exticr3;
	union syscfg__exticr exticr4;
	u32_t cfgr2;
	u32_t rsvd_0x1C;
	u32_t rsvd_0x20;
	u32_t rsvd_0x24;
	u32_t rsvd_0x28;
	u32_t rsvd_0x2C;
	u32_t rsvd_0x30;
	u32_t rsvd_0x34;
	u32_t rsvd_0x38;
	u32_t rsvd_0x3C;
	u32_t rsvd_0x40;
	u32_t rsvd_0x44;
	u32_t rsvd_0x48;
	u32_t rsvd_0x4C;
	u32_t cfgr3;
};

#endif /* _STM32F3X_GPIO_REGISTERS_H_ */
