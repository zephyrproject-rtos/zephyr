/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_AD4170_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_AD4170_ADC_H_

#include <zephyr/dt-bindings/dt-util.h>

/* AD4170 Channel Map */
#define AD4170_ADC_AIN0            0
#define AD4170_ADC_AIN1            1
#define AD4170_ADC_AIN2            2
#define AD4170_ADC_AIN3            3
#define AD4170_ADC_AIN4            4
#define AD4170_ADC_AIN5            5
#define AD4170_ADC_AIN6            6
#define AD4170_ADC_AIN7            7
#define AD4170_ADC_AIN8            8
#define AD4170_ADC_TEMP_SENSOR     17
#define AD4130_ADC_AVDD_AVSS_DIV5  18
#define AD4130_ADC_IOVDD_DGND_DIV5 19
#define AD4170_ADC_ALDO            21
#define AD4170_ADC_DLDO            22
#define AD4170_ADC_AVSS            23
#define AD4170_ADC_DGND            24
#define AD4170_ADC_REFIN1_PLUS     25
#define AD4170_ADC_REFIN1_MINUS    26
#define AD4170_ADC_REFIN2_PLUS     27
#define AD4170_ADC_REFIN2_MINUS    28
#define AD4170_ADC_REFOUT          29

/* AD4170 ADC Operating Mode */
#define AD4170_CONTINUOUS_MODE 0
#define AD4170_SINGLE_MODE     4
#define AD4170_STANDBY_MODE    5
#define AD4170_POWER_DOWN_MODE 6
#define AD4170_IDLE_MODE       7

/* AD4170 Clock Select */
#define AD4170_CLKSEL_INT      0
#define AD4170_CLKSEL_INT_OUT  1
#define AD4170_CLKSEL_EXT      2
#define AD4170_CLKSEL_EXT_XTAL 3

/* AD4170 Filter Type */
#define AD4170_SINC5_AVG 0
#define AD4170_SINC5     1
#define AD4170_SINC3     2

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_AD4170_ADC_H_ */
