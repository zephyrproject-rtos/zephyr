/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ABOV32_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ABOV32_CLOCK_H_

/**
 * @name ABOV32 SCU clock identifiers
 *
 * Used as the clock-cells value when referencing the SCU clock controller,
 * e.g. `clocks = <&scu_clk ABOV32_CLOCK_PCLK>;`. Values match the ordering
 * of SCUCLK_SRC_e in the ABOV HLL (type/scu_clk_type.h).
 * @{
 */
#define ABOV32_CLOCK_HSE  0U
#define ABOV32_CLOCK_HSI  1U
#define ABOV32_CLOCK_LSI  2U
#define ABOV32_CLOCK_LSE  3U
#define ABOV32_CLOCK_PLL  4U
#define ABOV32_CLOCK_MCLK 5U
#define ABOV32_CLOCK_HCLK 6U
#define ABOV32_CLOCK_PCLK 7U
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_ABOV32_CLOCK_H_ */
