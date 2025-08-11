/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RX_H_

#define RX_CLOCKS_SOURCE_CLOCK_LOCO     0
#define RX_CLOCKS_SOURCE_CLOCK_HOCO     1
#define RX_CLOCKS_SOURCE_CLOCK_MAIN_OSC 2
#define RX_CLOCKS_SOURCE_CLOCK_SUBCLOCK 3
#define RX_CLOCKS_SOURCE_PLL            4
#define RX_CLOCKS_SOURCE_CLOCK_DISABLE  0xff

#define RX_IF_CLOCKS_SOURCE_CLOCK_HOCO 0
#define RX_IF_CLOCKS_SOURCE_CLOCK_LOCO 2
#define RX_IF_CLOCKS_SOURCE_PLL        5
#define RX_IF_CLOCKS_SOURCE_PLL2       6

#define RX_LPT_CLOCKS_SOURCE_CLOCK_SUBCLOCK       0
#define RX_LPT_CLOCKS_SOURCE_CLOCK_IWDT_LOW_SPEED 1
#define RX_LPT_CLOCKS_NON_USE                     2
#define RX_LPT_CLOCKS_SOURCE_CLOCK_LOCO           3

#define RX_PLL_MUL_4   7
#define RX_PLL_MUL_4_5 8
#define RX_PLL_MUL_5   9
#define RX_PLL_MUL_5_5 10
#define RX_PLL_MUL_6   11
#define RX_PLL_MUL_6_5 12
#define RX_PLL_MUL_7   13
#define RX_PLL_MUL_7_5 14
#define RX_PLL_MUL_8   15

#define MSTPA 0
#define MSTPB 1
#define MSTPC 2
#define MSTPD 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RX_H_ */
