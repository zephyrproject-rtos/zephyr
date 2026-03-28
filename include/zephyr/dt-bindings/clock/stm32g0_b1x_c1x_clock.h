/*
 * Copyright (c) 2025 Andreas Schuster <andreas.schuster@schuam.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_B1X_C1X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_B1X_C1X_CLOCK_H_

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_256  8
#define MCO_PRE_DIV_512  9
#define MCO_PRE_DIV_1024 10

/* MCO clock output */
#define MCO_SEL_HSI48     2
#define MCO_SEL_PLLPCLK   8
#define MCO_SEL_PLLQCLK   9
#define MCO_SEL_RTCCLK    10
#define MCO_SEL_RTCWAKEUP 11

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G0_B1X_C1X_CLOCK_H_ */
