/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_PCNT_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_PCNT_ESP32_H_

#include <zephyr/drivers/sensor.h>

/**
 * @file
 * @brief ESP32 PCNT driver public extensions.
 *
 * Helpers for addressing a specific PCNT unit through the legacy
 * sensor_attr_set()/sensor_attr_get()/sensor_trigger_set() APIs, which
 * do not natively carry a channel index. Multi-unit access through the
 * Read-and-Decode API uses sensor_chan_spec.chan_idx directly and does
 * not need these helpers.
 */

/**
 * @brief Produce a sensor_channel value selecting PCNT unit @p n.
 *
 * Pass as the @c chan argument of sensor_attr_set/attr_get or the
 * @c chan field of struct sensor_trigger to target unit @p n. Unit 0
 * may also be addressed with SENSOR_CHAN_ROTATION or
 * SENSOR_CHAN_ENCODER_COUNT.
 */
#define SENSOR_CHAN_PCNT_ESP32_UNIT(n) ((enum sensor_channel)(SENSOR_CHAN_PRIV_START + (n)))

/**
 * @brief PCNT-specific sensor attributes.
 */
enum sensor_attribute_pcnt_esp32 {
	/**
	 * Clear the hardware counter of the addressed unit and reset the
	 * cached value to zero. The @c val argument is ignored.
	 */
	SENSOR_ATTR_PCNT_ESP32_RESET = SENSOR_ATTR_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_PCNT_ESP32_H_ */
