/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F0X_GPIO_REGISTERS_H_
#define _STM32F0X_GPIO_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F030x4/x6/x8/xC,
 *   STM32F070x6/xB advanced ARM ® -based MCUs
 *
 * Chapter 8: General-purpose I/Os (GPIO)
 * Chapter 9: System configuration controller (SYSCFG)
 */

struct stm32f0x_gpio {
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
		u32_t rsvd__2_7 :6 __packed;
		u32_t adc_dma_rmp :1 __packed;
		u32_t usart1_tx_dma_rmp :1 __packed;
		u32_t usart1_rx_dma_rmp :1 __packed;
		u32_t tim16_dma_rmp :1 __packed;
		u32_t tim17_dma_rmp :1 __packed;
		u32_t rsvd__13_15 :3 __packed;
		u32_t i2c_pb6_fmp :1 __packed;
		u32_t i2c_pb7_fmp :1 __packed;
		u32_t i2c_pb8_fmp :1 __packed;
		u32_t i2c_pb9_fmp :1 __packed;
		u32_t i2c1_fmp :1 __packed;
		u32_t rsvd__21 :1 __packed;
		u32_t i2c_pa9_fmp :1 __packed;
		u32_t i2c_pa10_fmp :1 __packed;
		u32_t rsvd__24_25 :2 __packed;
		u32_t usart3_dma_rmp :1 __packed;
		u32_t rsvd__27_31 :5 __packed;
	} bit;
};

union syscfg__exticr {
	u32_t val;
	struct {
		u16_t exti;
		u16_t rsvd__16_31;
	} bit;
};

struct stm32f0x_syscfg {
	union syscfg_cfgr1 cfgr1;
	u32_t rsvd;
	union syscfg__exticr exticr1;
	union syscfg__exticr exticr2;
	union syscfg__exticr exticr3;
	union syscfg__exticr exticr4;
	u32_t cfgr2;
};

#endif /* _STM32F0X_GPIO_REGISTERS_H_ */
