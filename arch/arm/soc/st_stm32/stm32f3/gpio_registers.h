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
 *   STM32F398xE advanced ARM ® -based MCUs
 *
 * Chapter 11: General-purpose I/Os
 */

struct stm32f3x_gpio {
	uint32_t moder;
	uint32_t otyper;
	uint32_t ospeedr;
	uint32_t pupdr;
	uint32_t idr;
	uint32_t odr;
	uint32_t bsrr;
	uint32_t lckr;
	uint32_t afrl;
	uint32_t afrh;
	uint32_t brr;
};

union syscfg_cfgr1 {
	uint32_t val;
	struct {
		uint32_t mem_mode :2 __packed;
		uint32_t rsvd__2_5 :4 __packed;
		uint32_t tim1_itr3_rmo :1 __packed;
		uint32_t dac_trig_rmp :1 __packed;
		uint32_t rsvd__8_10 :3 __packed;
		uint32_t tim16_dma_rmp :1 __packed;
		uint32_t tim17_dma_rmp :1 __packed;
		uint32_t tim16_dac1_dma_rmp :1 __packed;
		uint32_t tim17_dac2_dma_rmp :1 __packed;
		uint32_t dac2_ch1_dma_rmp :1 __packed;
		uint32_t i2c_pb6_fmp :1 __packed;
		uint32_t i2c_pb7_fmp :1 __packed;
		uint32_t i2c_pb8_fmp :1 __packed;
		uint32_t i2c_pb9_fmp :1 __packed;
		uint32_t i2c1_fmp :1 __packed;
		uint32_t rsvd__21 :1 __packed;
		uint32_t encoder_mode :2 __packed;
		uint32_t rsvd__24_25 :2 __packed;
		uint32_t fpu_ie :6 __packed;
	} bit;
};

union syscfg_rcr {
	uint32_t val;
	struct {
		uint32_t page0_wp :1 __packed;
		uint32_t page1_wp :1 __packed;
		uint32_t page2_wp :1 __packed;
		uint32_t page3_wp :1 __packed;
		uint32_t rsvd__4_31 :28 __packed;
	} bit;
};

union syscfg__exticr {
	uint32_t val;
	struct {
		uint16_t exti;
		uint16_t rsvd__16_31;
	} bit;
};

struct stm32f3x_syscfg {
	union syscfg_cfgr1 cfgr1;
	union syscfg_rcr rcr;
	union syscfg__exticr exticr1;
	union syscfg__exticr exticr2;
	union syscfg__exticr exticr3;
	union syscfg__exticr exticr4;
	uint32_t cfgr2;
	uint32_t rsvd_0x1C;
	uint32_t rsvd_0x20;
	uint32_t rsvd_0x24;
	uint32_t rsvd_0x28;
	uint32_t rsvd_0x2C;
	uint32_t rsvd_0x30;
	uint32_t rsvd_0x34;
	uint32_t rsvd_0x38;
	uint32_t rsvd_0x3C;
	uint32_t rsvd_0x40;
	uint32_t rsvd_0x44;
	uint32_t rsvd_0x48;
	uint32_t rsvd_0x4C;
	uint32_t cfgr3;
};

#endif /* _STM32F3X_GPIO_REGISTERS_H_ */
