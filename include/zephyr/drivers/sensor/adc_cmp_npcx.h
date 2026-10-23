/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of NPCX ADC comparator
 * @ingroup adc_cmp_npcx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_CMP_NPCX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_CMP_NPCX_H_

/**
 * @brief Nuvoton NPCX ADC threshold comparator
 * @defgroup adc_cmp_npcx_interface NPCX ADC comparator
 * @ingroup sensor_interface_ext_nuvoton
 * @{
 */

/** @brief Comparison between the ADC data and the threshold that triggers an event */
enum adc_cmp_npcx_comparison {
	/** ADC data is greater than the threshold */
	ADC_CMP_NPCX_GREATER,
	/** ADC data is less than or equal to the threshold */
	ADC_CMP_NPCX_LESS_OR_EQUAL,
};

/** @brief ADC threshold controllers in the NPCX series */
enum npcx_adc_cmp_thrctl {
	ADC_CMP_NPCX_THRCTL1,      /**< Threshold controller 1 */
	ADC_CMP_NPCX_THRCTL2,      /**< Threshold controller 2 */
	ADC_CMP_NPCX_THRCTL3,      /**< Threshold controller 3 */
	ADC_CMP_NPCX_THRCTL4,      /**< Threshold controller 4 */
	ADC_CMP_NPCX_THRCTL5,      /**< Threshold controller 5 */
	ADC_CMP_NPCX_THRCTL6,      /**< Threshold controller 6 */
	ADC_CMP_NPCX_THRCTL_COUNT, /**< Number of threshold controllers */
};

/** @brief Custom sensor attributes for the NPCX ADC comparator */
enum adc_cmp_npcx_sensor_attribute {
	/** Lower threshold, in mV */
	SENSOR_ATTR_LOWER_VOLTAGE_THRESH = SENSOR_ATTR_PRIV_START,
	/** Upper threshold, in mV */
	SENSOR_ATTR_UPPER_VOLTAGE_THRESH,
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_CMP_NPCX_H_ */
