/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Linaro Ltd.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32WB_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32WB_ADC_H_

#include <zephyr/dt-bindings/adc/stm32_adc.h>

#define STM32_ADC_RES_12	STM32_ADC_RES(12, 0, 3)
#define STM32_ADC_RES_10	STM32_ADC_RES(10, 1, 3)
#define STM32_ADC_RES_8		STM32_ADC_RES(8, 2, 3)
#define STM32_ADC_RES_6		STM32_ADC_RES(6, 3, 3)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_STM32WB_ADC_H_ */
