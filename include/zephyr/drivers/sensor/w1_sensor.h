/*
 * Copyright (c) 2022, Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for 1-Wire sensors
 * @ingroup w1_sensor
 *
 * This header exposes a sensor attribute and helper functions to allow the
 * runtime configuration of ROM IDs for 1-Wire sensors.
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
 * @ingroup sensor_interface_ext
 * @{
 */

/**
 * @brief Extended sensor attributes for 1-Wire sensors.
 */
enum sensor_attribute_w1 {
	/**
	 * Device unique 1-Wire ROM identifier.
	 *
	 * 64-bit ROM (family code, serial number, and CRC) encoded as a
	 * big-endian integer:
	 *
	 * - @c sensor_value.val1 is the lower 32 bits.
	 * - @c sensor_value.val2 is the upper 32 bits.
	 *
	 * Use w1_rom_to_sensor_value() and w1_sensor_value_to_rom() to convert
	 * between @ref w1_rom and @ref sensor_value.
	 */
	SENSOR_ATTR_W1_ROM = SENSOR_ATTR_PRIV_START,
};

/**
 * @brief Encode a @ref w1_rom as a @ref sensor_value.
 *
 * @param[in] rom   Pointer to the ROM structure.
 * @param[out] val  Pointer to the sensor value to populate.
 */
static inline void w1_rom_to_sensor_value(const struct w1_rom *rom,
					  struct sensor_value *val)
{
	val->val1 = sys_get_be64((uint8_t *)rom) & BIT64_MASK(32);
	val->val2 = sys_get_be64((uint8_t *)rom) >> 32;
}

/**
 * @brief Decode a @ref sensor_value into a @ref w1_rom.
 *
 * @param[in]  val  Pointer to the sensor value containing the encoded ROM.
 * @param[out] rom  Pointer to the ROM structure to populate.
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
