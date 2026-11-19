/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCG_LITE_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCG_LITE_H_

/** MCGOUTCLK, same value as KINETIS_MCG_OUT_CLK */
#define NXP_MCG_LITE_OUT_CLK       1
/** MCGIRCLK: LIRC divided by FCRDIV and LIRC_DIV2 */
#define NXP_MCG_LITE_IRCLK         2
/** MCGPCLK: HIRC 48 MHz for peripherals */
#define NXP_MCG_LITE_PCLK          3
/** LIRC_DIV1: LIRC divided by FCRDIV */
#define NXP_MCG_LITE_LIRC_DIV1_CLK 4

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_MCG_LITE_H_ */
