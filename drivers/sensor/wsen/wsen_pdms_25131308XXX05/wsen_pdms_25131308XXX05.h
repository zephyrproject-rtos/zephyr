/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_PDMS_25131308XXX05_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_PDMS_25131308XXX05_H_

#include <stdbool.h>

#include <zephyr/drivers/sensor.h>

#include <platform.h>

#include <WSEN_PDMS_25131308XXX05.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

struct pdms_25131308XXX05_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	PDMS_Spi_CrcSelect_t spi_crc;
#endif

	uint16_t pressure_data;
	uint16_t temperature_data;
};

struct pdms_25131308XXX05_config {
	union {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
		const struct i2c_dt_spec i2c;
#endif
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
		const struct spi_dt_spec spi;
#endif
	} bus_cfg;

	const PDMS_SensorType_t sensor_type;
	const bool crc;
};

/* GCDS Sensor API functions*/
static int pdms_25131308XXX05_sample_fetch(const struct device *dev, enum sensor_channel chan);
static int pdms_25131308XXX05_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val);

static int pdms_25131308XXX05_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_PDMS_25131308XXX05_H_ */
