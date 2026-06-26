/*
 * Copyright (c) 2025 Andreas Schuster <andreas.schuster@schuam.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_B1X_C1X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_B1X_C1X_CLOCK_H_

/* MCO prescaler : division factor */
/** @brief MCO prescaler division factor: 256 */
#define MCO_PRE_DIV_256  8
/** @brief MCO prescaler division factor: 512 */
#define MCO_PRE_DIV_512  9
/** @brief MCO prescaler division factor: 1024 */
#define MCO_PRE_DIV_1024 10

/* MCO clock output */
/** @brief MCO clock source selection value: HSI48 */
#define MCO_SEL_HSI48     2
/** @brief MCO clock source selection value: PLLPCLK */
#define MCO_SEL_PLLPCLK   8
/** @brief MCO clock source selection value: PLLQCLK */
#define MCO_SEL_PLLQCLK   9
/** @brief MCO clock source selection value: RTCCLK */
#define MCO_SEL_RTCCLK    10
/** @brief MCO clock source selection value: RTCWAKEUP */
#define MCO_SEL_RTCWAKEUP 11

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_B1X_C1X_CLOCK_H_ */
