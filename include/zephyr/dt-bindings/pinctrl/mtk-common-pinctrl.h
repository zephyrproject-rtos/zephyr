/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MTK_COMMON_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MTK_COMMON_PINCTRL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A pinmux entry packs the pin number and the function selected on it into a
 * single cell: the pin number is held above the low four bits, which hold the
 * function.
 */
#define MTK_PIN_NO(x)       ((x) << 8)
#define MTK_GET_PIN_NO(x)   ((x) >> 8)
#define MTK_GET_PIN_FUNC(x) ((x) & 0xf)

#define MTK_PUPD_SET_R1R0_00 100
#define MTK_PUPD_SET_R1R0_01 101
#define MTK_PUPD_SET_R1R0_10 102
#define MTK_PUPD_SET_R1R0_11 103
#define MTK_RSEL_SET_R1R0_00 104
#define MTK_RSEL_SET_R1R0_01 105
#define MTK_RSEL_SET_R1R0_10 106
#define MTK_RSEL_SET_R1R0_11 107

#define MTK_PULL_SET_RSEL_000 200
#define MTK_PULL_SET_RSEL_001 201
#define MTK_PULL_SET_RSEL_010 202
#define MTK_PULL_SET_RSEL_011 203
#define MTK_PULL_SET_RSEL_100 204
#define MTK_PULL_SET_RSEL_101 205
#define MTK_PULL_SET_RSEL_110 206
#define MTK_PULL_SET_RSEL_111 207

#define MTK_DRIVE_2mA  2
#define MTK_DRIVE_4mA  4
#define MTK_DRIVE_6mA  6
#define MTK_DRIVE_8mA  8
#define MTK_DRIVE_10mA 10
#define MTK_DRIVE_12mA 12
#define MTK_DRIVE_14mA 14
#define MTK_DRIVE_16mA 16
#define MTK_DRIVE_20mA 20
#define MTK_DRIVE_24mA 24
#define MTK_DRIVE_28mA 28
#define MTK_DRIVE_32mA 32

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_MTK_COMMON_PINCTRL_H_ */
