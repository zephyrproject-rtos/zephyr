/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32_TEMP_H_
#define _STM32_TEMP_H_

#include <stdint.h>

#include <device.h>
#include <soc.h>

struct temp_stm32_config {
	ADC_TypeDef *adc;
	ADC_Common_TypeDef *adc_common;
	u8_t adc_channel;
	struct stm32_pclken pclken;
};

struct temp_stm32_data {
	struct device *clock;
};

#endif	/* _STM32_TEMP_H_ */
