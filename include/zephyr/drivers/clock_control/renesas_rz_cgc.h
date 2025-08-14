/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CGC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CGC_H_

#include <zephyr/drivers/clock_control.h>

#define RZ_CGC_SUBCLK_DIV(subclk)                                                                  \
	UTIL_CAT(RZ_CGC_DIV_, DT_NODE_FULL_NAME_UPPER_TOKEN(subclk))                               \
	(DT_PROP(subclk, div))

#define RZ_CGC_SUBCLK_MUL(subclk)                                                                  \
	UTIL_CAT(RZ_CGC_MUL_, DT_NODE_FULL_NAME_UPPER_TOKEN(subclk))                               \
	(DT_PROP(subclk, mul))

#define RZ_CGC_DIV_CKIO(n)    UTIL_CAT(BSP_CLOCKS_CKIO_ICLK_DIV, n)
#define RZ_CGC_MUL_CPU0CLK(n) UTIL_CAT(BSP_CLOCKS_FSELCPU0_ICLK_MUL, n)
#define RZ_CGC_MUL_CPU1CLK(n) UTIL_CAT(BSP_CLOCKS_FSELCPU1_ICLK_MUL, n)

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_RENESAS_RZ_CGC_H_ */
