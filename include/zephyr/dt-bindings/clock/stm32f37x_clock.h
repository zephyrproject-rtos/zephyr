/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F37X_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F37X_CLOCK_H_

#include "stm32f3_clock.h"

/* On STM32F37x, the ADC prescaler is located in CFGR1 and the prescaler values are more limited */
#undef ADC12_PRE
#undef ADC34_PRE
#undef ADC_PRE_DISABLED
#undef ADC_PRE_DIV_1
#undef ADC_PRE_DIV_2
#undef ADC_PRE_DIV_4
#undef ADC_PRE_DIV_6
#undef ADC_PRE_DIV_8
#undef ADC_PRE_DIV_10
#undef ADC_PRE_DIV_12
#undef ADC_PRE_DIV_16
#undef ADC_PRE_DIV_32
#undef ADC_PRE_DIV_64
#undef ADC_PRE_DIV_128
#undef ADC_PRE_DIV_256

/** @brief Device domain clocks selection helpers */
/** CFGR devices */
#define ADC_PRE(val)		STM32_DT_CLOCK_SELECT((val), 15, 14, CFGR_REG)

/* ADC prescaler division factor for STM32F37x */
#define ADC_PRE_DIV_2		0
#define ADC_PRE_DIV_4		1
#define ADC_PRE_DIV_6		2
#define ADC_PRE_DIV_8		3

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32F37X_CLOCK_H_ */
