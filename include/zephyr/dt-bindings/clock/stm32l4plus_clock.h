/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4PLUS_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4PLUS_CLOCK_H_

#include "stm32l4_clock.h"

/*
 * On STM32L4+ series, the SAI1 / SAI2 input clock selection fields
 * are located within the CCIPR2 register instead of the CCIPR register
 */
#undef SAI1_SEL(val)
#undef SAI2_SEL(val)

/** CCIPR2 devices */
#define SAI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 5, CCIPR2_REG)
#define SAI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 8, CCIPR2_REG)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32L4PLUS_CLOCK_H_ */
