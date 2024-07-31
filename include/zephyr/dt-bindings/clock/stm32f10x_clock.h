/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2024, Joakim Andersson
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F10X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F10X_CLOCK_H_

#include "stm32_common_clocks.h"
/* Ensure correct order by including generic F1 definitions first. */
#include "stm32f1_clock.h"

/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
/* Common clocks with stm32f1x defined in stm32f1_clock.h */
#define STM32_SRC_PLL2CLK           (STM32_SRC_PLLCLK + 1)
#define STM32_SRC_PLL3CLK           (STM32_SRC_PLL2CLK + 1)
#define STM32_SRC_EXT_HSE           (STM32_SRC_PLL3CLK + 1)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F10X_CLOCK_H_ */
