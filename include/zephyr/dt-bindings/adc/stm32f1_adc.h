/*
 * Copyright (c) 2023 STMicrelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_STM32F1_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_STM32F1_ADC_H_

#include <zephyr/dt-bindings/adc/stm32_adc.h>

/*
 * For STM32 F1 and similar, the only available resolution is 12-bit
 * and there is no register to set it.
 * We still need the macro to get the value of the resolution but the driver
 * does not set the resolution in any register by checking that the register
 * address is configured as 0xFF
 */
#define STM32_ADC_RES_REG	0xFF
#define STM32_ADC_RES_SHIFT	0
#define STM32_ADC_RES_MASK	0x00
#define STM32_ADC_RES_REG_VAL	0x00

#define STM32F1_ADC_RES(resolution)	\
	STM32_ADC_RES(resolution, STM32_ADC_RES_REG_VAL)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_STM32F1_ADC_H_ */
