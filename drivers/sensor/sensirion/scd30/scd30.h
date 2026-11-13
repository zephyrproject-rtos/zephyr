/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SCD30_SCD30_H_
#define ZEPHYR_DRIVERS_SENSOR_SCD30_SCD30_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#define SCD30_CMD_START_PERIODIC_MEASUREMENT 0x0010
#define SCD30_CMD_STOP_PERIODIC_MEASUREMENT  0x0104
#define SCD30_CMD_SET_MEASUREMENT_INTERVAL   0x4600
#define SCD30_CMD_GET_DATA_READY             0x0202
#define SCD30_CMD_READ_MEASUREMENT           0x0300
#define SCD30_CMD_SET_ASC                    0x5306
#define SCD30_CMD_SET_FRC                    0x5204
#define SCD30_CMD_SET_TEMPERATURE_OFFSET     0x5403
#define SCD30_CMD_SET_ALTITUDE               0x5102
#define SCD30_CMD_GET_FIRMWARE_VERSION       0xD100
#define SCD30_CMD_SOFT_RESET                 0xD304

#define SCD30_CRC_POLY 0x31
#define SCD30_CRC_INIT 0xFF

/* Datasheet: boot-up time < 2 s */
#define SCD30_BOOT_TIME_MS 2000
/* Common practice after stop before issuing further commands */
#define SCD30_STOP_DELAY_MS 500
/* Datasheet: wait > 3 ms after write before read */
#define SCD30_CMD_DELAY_MS 10

#define SCD30_MEASUREMENT_INTERVAL_MIN_S 2
#define SCD30_MEASUREMENT_INTERVAL_MAX_S 1800
#define SCD30_AMBIENT_PRESSURE_MIN       700
#define SCD30_AMBIENT_PRESSURE_MAX       1400
#define SCD30_FRC_CONCENTRATION_MIN      400
#define SCD30_FRC_CONCENTRATION_MAX      2000

struct scd30_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec ready_gpio;
	uint16_t measurement_interval;
};

struct scd30_data {
	float co2_sample;
	float temp_sample;
	float humi_sample;
	uint16_t ambient_pressure;
	bool measuring;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SCD30_SCD30_H_ */
