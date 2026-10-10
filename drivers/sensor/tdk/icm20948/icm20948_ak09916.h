/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20948_AK09916_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20948_AK09916_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "icm20948.h"

/*
 * Brings up the magnetometer behind the auxiliary I2C bus of the device and
 * arms the periodic readout into EXT_SLV_SENS_DATA. Must be called once the
 * device itself is out of sleep, as the auxiliary bus runs off its clock.
 */
int ak09916_init(const struct device *dev);

/*
 * Takes the nine bytes the periodic readout mirrors into EXT_SLV_SENS_DATA
 * and stores them if they hold a measurement that has not been read yet.
 */
void ak09916_parse_magn(struct icm20948_data *data, const uint8_t *ext);

/* Returns -EOVERFLOW if st2 reports that the measurement went out of range */
int ak09916_convert_magn(struct sensor_value *val, int16_t raw, uint8_t st2);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20948_AK09916_H_ */
