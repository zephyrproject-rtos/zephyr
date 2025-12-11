/*
 * Copyright (c) 2022, Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for 1-Wire Sensors
 *
 * This header file exposes an attribute an helper function to allow the
 * runtime configuration of ROM IDs for 1-Wire Sensors.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_W1_SENSOR_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_W1_SENSOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/w1.h>

/**
 * @brief 1-Wire Sensor API
 * @defgroup w1_sensor 1-Wire Sensor API
 * @ingroup w1_interface
 * @{
 */

enum sensor_attribute_w1 {
	/** Device unique 1-Wire ROM */
	SENSOR_ATTR_W1_ROM = SENSOR_ATTR_PRIV_START,
};

/**
 * @brief Function to write a w1_rom struct to an sensor value ptr.
 *
 * @param rom  Pointer to the rom struct.
 * @param val  Pointer to the sensor value.
 */
static inline void w1_rom_to_sensor_value(const struct w1_rom *rom,
					  struct sensor_value *val)
{
	val->val1 = sys_get_be64((uint8_t *)rom) & BIT64_MASK(32);
	val->val2 = sys_get_be64((uint8_t *)rom) >> 32;
}

/**
 * @brief Function to write an rom id stored in a sensor value to a struct w1_rom ptr.
 *
 * @param val  Sensor_value representing the rom.
 * @param rom  The rom struct ptr.
 */
static inline void w1_sensor_value_to_rom(const struct sensor_value *val,
					  struct w1_rom *rom)
{
	uint64_t temp64 = ((uint64_t)((uint32_t)val->val2) << 32)
			  | (uint32_t)val->val1;
	sys_put_be64(temp64, (uint8_t *)rom);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_W1_SENSOR_H_ */
