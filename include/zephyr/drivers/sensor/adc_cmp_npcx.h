/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ADC_CMP_NPCX_H_
#define _ADC_CMP_NPCX_H_

enum adc_cmp_npcx_comparison {
	ADC_CMP_NPCX_GREATER,
	ADC_CMP_NPCX_LESS_OR_EQUAL,
};

enum adc_cmp_npcx_sensor_attribute {
	SENSOR_ATTR_LOWER_VOLTAGE_THRESH = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_UPPER_VOLTAGE_THRESH,
};

#endif /* _ADC_CMP_NPCX_H_ */
