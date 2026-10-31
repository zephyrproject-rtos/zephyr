/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of NPCX ADC voltage-to-temperature sensor
 * @ingroup adc_v2t_npcx_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_V2T_NPCX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_V2T_NPCX_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Nuvoton NPCX ADC voltage-to-temperature (V2T) sensor
 * @defgroup adc_v2t_npcx_interface NPCX ADC V2T
 * @ingroup sensor_interface_ext_nuvoton
 * @{
 */

/** @brief Custom sensor attributes for the NPCX ADC V2T sensor */
enum npcx_adc_v2t_sensor_attr {
	/** Bit mask of the ADC channels used for V2T conversion */
	SENSOR_ATTR_NPCX_V2T_CHANNEL_CFG = SENSOR_ATTR_PRIV_START,
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_V2T_NPCX_H_ */
