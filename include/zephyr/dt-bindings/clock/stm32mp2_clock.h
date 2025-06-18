/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_

#include "stm32_common_clocks.h"

/* Undefine the common clocks macro */
#undef STM32_CLOCK

/**
 * Pack RCC clock register offset and bit in two 32-bit values
 * as expected for the Device Tree `clocks` property on STM32.
 *
 * @param per STM32 Peripheral name (expands to STM32_CLOCK_PERIPH_{PER})
 * @param bit Clock bit
 */
#define STM32_CLOCK(per, bit) (STM32_CLOCK_PERIPH_##per) (1 << bit)

/* Clock reg */
#define STM32_CLK		1U
#define STM32_LP_CLK		2U

/* GPIO Peripheral */
#define STM32_CLOCK_PERIPH_GPIOA	0x52C
#define STM32_CLOCK_PERIPH_GPIOB	0x530
#define STM32_CLOCK_PERIPH_GPIOC	0x534
#define STM32_CLOCK_PERIPH_GPIOD	0x538
#define STM32_CLOCK_PERIPH_GPIOE	0x53C
#define STM32_CLOCK_PERIPH_GPIOF	0x540
#define STM32_CLOCK_PERIPH_GPIOG	0x544
#define STM32_CLOCK_PERIPH_GPIOH	0x548
#define STM32_CLOCK_PERIPH_GPIOI	0x54C
#define STM32_CLOCK_PERIPH_GPIOJ	0x550
#define STM32_CLOCK_PERIPH_GPIOK	0x554
#define STM32_CLOCK_PERIPH_GPIOZ	0x558

#define STM32_CLOCK_PERIPH_MIN	STM32_CLOCK_PERIPH_GPIOA
#define STM32_CLOCK_PERIPH_MAX	STM32_CLOCK_PERIPH_GPIOZ

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_ */
