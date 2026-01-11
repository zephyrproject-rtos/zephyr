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

#ifdef CONFIG_SOC_SERIES_RX26T
#define RX_PLL_CLOCKS_SOURCE_CLOCK_MAIN_OSC 0
#define RX_PLL_CLOCKS_SOURCE_CLOCK_HOCO     1
#endif /* CONFIG_SOC_SERIES_RX26T */

#define RX_PLL_MUL_4   7
#define RX_PLL_MUL_4_5 8
#define RX_PLL_MUL_5   9
#define RX_PLL_MUL_5_5 10
#define RX_PLL_MUL_6   11
#define RX_PLL_MUL_6_5 12
#define RX_PLL_MUL_7   13
#define RX_PLL_MUL_7_5 14
#define RX_PLL_MUL_8   15

#define RX_PLL_MUL_10   19
#define RX_PLL_MUL_10_5 20
#define RX_PLL_MUL_11   21
#define RX_PLL_MUL_11_5 22
#define RX_PLL_MUL_12   23
#define RX_PLL_MUL_12_5 24
#define RX_PLL_MUL_13   25
#define RX_PLL_MUL_13_5 26
#define RX_PLL_MUL_14   27
#define RX_PLL_MUL_14_5 28
#define RX_PLL_MUL_15   29
#define RX_PLL_MUL_15_5 30
#define RX_PLL_MUL_16   31
#define RX_PLL_MUL_16_5 32
#define RX_PLL_MUL_17   33
#define RX_PLL_MUL_17_5 34
#define RX_PLL_MUL_18   35
#define RX_PLL_MUL_18_5 36
#define RX_PLL_MUL_19   37
#define RX_PLL_MUL_19_5 38
#define RX_PLL_MUL_20   39
#define RX_PLL_MUL_20_5 40
#define RX_PLL_MUL_21   41
#define RX_PLL_MUL_21_5 42
#define RX_PLL_MUL_22   43
#define RX_PLL_MUL_22_5 44
#define RX_PLL_MUL_23   45
#define RX_PLL_MUL_23_5 46
#define RX_PLL_MUL_24   47
#define RX_PLL_MUL_24_5 48
#define RX_PLL_MUL_25   49
#define RX_PLL_MUL_25_5 50
#define RX_PLL_MUL_26   51
#define RX_PLL_MUL_26_5 52
#define RX_PLL_MUL_27   53
#define RX_PLL_MUL_27_5 54
#define RX_PLL_MUL_28   55
#define RX_PLL_MUL_28_5 56
#define RX_PLL_MUL_29   57
#define RX_PLL_MUL_29_5 58
#define RX_PLL_MUL_30   59

#define MSTPA 0
#define MSTPB 1
#define MSTPC 2
#define MSTPD 3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_RX_H_ */
