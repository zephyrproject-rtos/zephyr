/*
 * Copyright (c) 2023 STMicrelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_STM32U5_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_STM32U5_ADC_H_

#include <zephyr/dt-bindings/adc/stm32_adc.h>

/* STM32 ADC resolution register for U5 and similar */
#define STM32_ADC_RES_REG	0x0C
#define STM32_ADC_RES_SHIFT	2
#define STM32_ADC_RES_MASK	BIT_MASK(2)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_STM32U5_ADC_H_ */
