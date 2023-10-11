/*
 * Copyright (c) 2023 STMicrelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_STM32F4_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_STM32F4_ADC_H_

#include <zephyr/dt-bindings/adc/stm32_adc.h>

/* STM32 ADC resolution register for F4 and similar */
#define STM32_ADC_RES_REG	0x04
#define STM32_ADC_RES_SHIFT	24
#define STM32_ADC_RES_MASK	BIT_MASK(2)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_STM32F4_ADC_H_ */
