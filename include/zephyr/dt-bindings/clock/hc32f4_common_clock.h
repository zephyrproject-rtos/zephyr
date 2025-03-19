/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_HC32F4_COMMON_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_HC32F4_COMMON_CLOCK_H_

#define	HC32_CLK_FCG0		0
#define	HC32_CLK_FCG1		1
#define	HC32_CLK_FCG2		2
#define	HC32_CLK_FCG3		3
#define HC32_CLK_FCG_AVALID	0xFF

#define HC32_CLK_SRC_HRC				(0x00)
#define HC32_CLK_SRC_MRC				(0x01)
#define HC32_CLK_SRC_LRC				(0x02)
#define HC32_CLK_SRC_XTAL				(0x03)
#define HC32_CLK_SRC_XTAL32				(0x04)
#define HC32_CLK_SRC_PLL				(0x05)
#define HC32_CLK_BUS_HCLK				(0x07)
#define HC32_CLK_BUS_PCLK0				(0x08)
#define HC32_CLK_BUS_PCLK1				(0x09)
#define HC32_CLK_BUS_PCLK2				(0x0A)
#define HC32_CLK_BUS_PCLK3				(0x0B)
#define HC32_CLK_BUS_PCLK4				(0x0C)
#define HC32_SYS_CLK					(0x0D)

#define HC32_CLK_CONF_PERI				(0)
#define HC32_CLK_CONF_TPIU				(1)
#define HC32_CLK_CONF_SRC				(2)
#define HC32_CLK_CONF_MCO				(3)
#define HC32_CLK_CONF_MIN				HC32_CLK_CONF_PERI
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_HC32F4_COMMON_CLOCK_H_ */
