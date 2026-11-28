/*
 * SPDX-FileCopyrightText: Copyright RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of Winsen MQ series gas sensors
 * @ingroup mq_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MQ_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MQ_H_

/**
 * @defgroup mq_interface MQ
 * @ingroup sensor_interface_ext_winsen
 * @brief Winsen MQ series gas sensors
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor channels for the MQ series
 */
enum sensor_channel_mq {
	/**
	 * Gas concentration in parts per million.
	 *
	 * Derived from the sensor resistance and the resistance in clean air,
	 * so it is reported only once that reference is known.
	 */
	SENSOR_CHAN_MQ_PPM = SENSOR_CHAN_PRIV_START,
};

/**
 * @brief Custom sensor attributes for the MQ series
 */
enum sensor_attribute_mq {
	/**
	 * Resistance the sensing element shows in clean air, in ohms.
	 *
	 * The part is calibrated by leaving it in clean air, reading
	 * SENSOR_CHAN_GAS_RES and writing that value back here.
	 */
	SENSOR_ATTR_MQ_R0 = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MQ_H_ */
