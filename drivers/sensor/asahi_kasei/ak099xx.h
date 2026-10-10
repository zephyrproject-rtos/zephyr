/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Definitions shared by the AK099xx family of AKM magnetometers */

#ifndef ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK099XX_H_
#define ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK099XX_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util_macro.h>

/* WIA1 company ID */
#define AK099XX_WIA1_AKM 0x48

/* ST1 data ready bit */
#define AK099XX_ST1_DRDY BIT(0)

/* Operation mode codes */
#define AK099XX_MODE_POWER_DOWN 0x00
#define AK099XX_MODE_SINGLE     0x01
#define AK099XX_MODE_CONT_10HZ  0x02
#define AK099XX_MODE_CONT_20HZ  0x04
#define AK099XX_MODE_CONT_50HZ  0x06
#define AK099XX_MODE_CONT_100HZ 0x08
#define AK099XX_MODE_SELF_TEST  0x10

/* Minimum time in power-down mode before another mode can be set */
#define AK099XX_MODE_CHANGE_WAIT_US 100

/* Continuous measurement mode and its rate */
struct ak099xx_odr {
	uint16_t hz;
	uint8_t mode;
};

/* Continuous measurement modes available on every family member, slowest first */
#define AK099XX_ODR_COMMON                                                                         \
	{10, AK099XX_MODE_CONT_10HZ}, {20, AK099XX_MODE_CONT_20HZ},                                \
	{50, AK099XX_MODE_CONT_50HZ}, {100, AK099XX_MODE_CONT_100HZ}

/**
 * @brief Select the fastest mode of @p table whose rate does not exceed @p val.
 *
 * A rate below the slowest entry selects the slowest entry, and a rate below 1 Hz
 * selects power-down.
 *
 * @param table Continuous modes sorted by ascending rate
 * @param count Number of entries of @p table
 * @param val Requested sampling frequency
 *
 * @return Mode code
 */
static inline uint8_t ak099xx_odr_to_mode(const struct ak099xx_odr *table, size_t count,
					  const struct sensor_value *val)
{
	uint8_t mode = AK099XX_MODE_POWER_DOWN;

	if (val->val1 <= 0) {
		return mode;
	}

	for (size_t i = 0; i < count; i++) {
		if ((i > 0U) && (val->val1 < table[i].hz)) {
			break;
		}
		mode = table[i].mode;
	}

	return mode;
}

/**
 * @brief Get the sampling frequency of a mode.
 *
 * @param table Continuous modes
 * @param count Number of entries of @p table
 * @param mode Mode code
 * @param val Sampling frequency, 0 when @p mode is not a continuous mode of @p table
 */
static inline void ak099xx_mode_to_odr(const struct ak099xx_odr *table, size_t count, uint8_t mode,
				       struct sensor_value *val)
{
	val->val1 = 0;
	val->val2 = 0;

	for (size_t i = 0; i < count; i++) {
		if (table[i].mode == mode) {
			val->val1 = table[i].hz;
			break;
		}
	}
}

#endif /* ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK099XX_H_ */
