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

/* Supported ADC threshold controllers in NPCX series */
enum npcx_adc_cmp_thrctl {
	ADC_CMP_NPCX_THRCTL1,
	ADC_CMP_NPCX_THRCTL2,
	ADC_CMP_NPCX_THRCTL3,
	ADC_CMP_NPCX_THRCTL4,
	ADC_CMP_NPCX_THRCTL5,
	ADC_CMP_NPCX_THRCTL6,
	ADC_CMP_NPCX_THRCTL_COUNT,
};

enum adc_cmp_npcx_sensor_attribute {
	SENSOR_ATTR_LOWER_VOLTAGE_THRESH = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_UPPER_VOLTAGE_THRESH,
};

#endif /* _ADC_CMP_NPCX_H_ */
