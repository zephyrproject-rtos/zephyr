/*
 * Copyright (c) 2025 Srishtik Bhandarkar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of PZEM-004T sensor
 * @ingroup pzem004t_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_PZEM004T_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_PZEM004T_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Peacefair PZEM-004T AC energy meter
 * @defgroup pzem004t_interface PZEM-004T
 * @ingroup sensor_interface_ext_peacefair
 * @{
 */

/** @brief Custom sensor channels for PZEM-004T */
enum sensor_channel_pzem004t {
	/** Active energy accumulated over time, in Wh. */
	SENSOR_CHAN_PZEM004T_ENERGY = SENSOR_CHAN_PRIV_START,
	/** Power factor, the ratio of real power to apparent power, with 0.01 resolution. */
	SENSOR_CHAN_PZEM004T_POWER_FACTOR,
	/**
	 * Alarm status: 0xFF when the active power is above the power alarm threshold, 0x00
	 * otherwise.
	 */
	SENSOR_CHAN_PZEM004T_ALARM_STATUS,
	/** Active power alarm threshold, in W. */
	SENSOR_CHAN_PZEM004T_POWER_ALARM_THRESHOLD,
	/**
	 * Modbus address of the device. Only set it with a single device connected to the bus.
	 */
	SENSOR_CHAN_PZEM004T_MODBUS_RTU_ADDRESS,
	/**
	 * Modbus address the driver instance uses to reach the device. This does not change the
	 * address of the device itself.
	 */
	SENSOR_CHAN_PZEM004T_ADDRESS_INST_SET,
	/**
	 * Reset the energy counter of the device. Requires
	 * @kconfig{CONFIG_PZEM004T_ENABLE_RESET_ENERGY}.
	 */
	SENSOR_CHAN_PZEM004T_RESET_ENERGY,
};

/** @brief Custom sensor attributes for PZEM-004T */
enum sensor_attribute_pzem004t {
	/** Active power alarm threshold, in W. */
	SENSOR_ATTR_PZEM004T_POWER_ALARM_THRESHOLD = SENSOR_ATTR_PRIV_START,
	/**
	 * Modbus address of the device. Only set it with a single device connected to the bus.
	 */
	SENSOR_ATTR_PZEM004T_MODBUS_RTU_ADDRESS,
	/**
	 * Modbus address the driver instance uses to reach the device. This does not change the
	 * address of the device itself.
	 */
	SENSOR_ATTR_PZEM004T_ADDRESS_INST_SET,
	/**
	 * Reset the energy counter of the device. Requires
	 * @kconfig{CONFIG_PZEM004T_ENABLE_RESET_ENERGY}.
	 */
	SENSOR_ATTR_PZEM004T_RESET_ENERGY,
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_PZEM004T_H_ */
