/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_V2T_NPCX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_V2T_NPCX_H_

#include <zephyr/drivers/sensor.h>

/* V2T sensor attributes */
enum npcx_adc_v2t_sensor_attr {
	SENSOR_ATTR_NPCX_V2T_CHANNEL_CFG = SENSOR_ATTR_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADC_V2T_NPCX_H_ */
