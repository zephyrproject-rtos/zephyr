/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_SYSCFG_REGISTERS_H_
#define _STM32_SYSCFG_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F030x4/x6/x8/xC,
 *   STM32F070x6/xB advanced ARM ® -based MCUs
 *
 * Chapter 9: System configuration controller (SYSCFG)
 */

#include "../common/soc_syscfg_common.h"

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

struct stm32_syscfg {
	union syscfg_cfgr1 cfgr1;
	u32_t rsvd;
	union syscfg_exticr exticr1;
	union syscfg_exticr exticr2;
	union syscfg_exticr exticr3;
	union syscfg_exticr exticr4;
	u32_t cfgr2;
};

#endif /* _STM32_SYSCFG_REGISTERS_H_ */
