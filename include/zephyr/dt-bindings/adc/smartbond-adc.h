/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_SMARTBOND_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_SMARTBOND_ADC_H_

#include <zephyr/dt-bindings/dt-util.h>

/* Values for DT zephyr,input-positive */
#define SMARTBOND_GPADC_P1_09		0
#define SMARTBOND_GPADC_P0_25		1
#define SMARTBOND_GPADC_P0_08		2
#define SMARTBOND_GPADC_P0_09		3
#define SMARTBOND_GPADC_VDD		4
#define SMARTBOND_GPADC_V30		5
#define SMARTBOND_GPADC_VBAT		8
#define SMARTBOND_GPADC_VSSA		9
#define SMARTBOND_GPADC_P1_13		16
#define SMARTBOND_GPADC_P1_12		17
#define SMARTBOND_GPADC_P1_18		18
#define SMARTBOND_GPADC_P1_19		19
#define SMARTBOND_GPADC_TEMP		20

/* Values for DT zephyr,input-positive or input-negative */
#define SMARTBOND_SDADC_P1_09		0
#define SMARTBOND_SDADC_P0_25		1
#define SMARTBOND_SDADC_P0_08		2
#define SMARTBOND_SDADC_P0_09		3
#define SMARTBOND_SDADC_P1_14		4
#define SMARTBOND_SDADC_P1_20		5
#define SMARTBOND_SDADC_P1_21		6
#define SMARTBOND_SDADC_P1_22		7
/* Values for DT zephyr,input-positive only */
#define SMARTBOND_SDADC_VBAT		8

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_SMARTBOND_ADC_H_ */
