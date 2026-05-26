/*
 * Copyright (c) 2026 Shi Weizhi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SPS30_SPS30_INTERNAL_H_
#define ZEPHYR_DRIVERS_SENSOR_SPS30_SPS30_INTERNAL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/* I2C address (fixed) */
#define SPS30_I2C_ADDR 0x69

/* I2C command pointer addresses (datasheet §6.3) */
#define SPS30_PTR_START_MEASUREMENT      0x0010
#define SPS30_PTR_STOP_MEASUREMENT       0x0104
#define SPS30_PTR_READ_DATA_READY        0x0202
#define SPS30_PTR_READ_MEASURED_VALUES   0x0300
#define SPS30_PTR_SLEEP                  0x1001
#define SPS30_PTR_WAKE_UP                0x1103
#define SPS30_PTR_START_FAN_CLEANING     0x5607
#define SPS30_PTR_AUTO_CLEANING_INTERVAL 0x8004
#define SPS30_PTR_READ_PRODUCT_TYPE      0xD002
#define SPS30_PTR_READ_SERIAL_NUMBER     0xD033
#define SPS30_PTR_READ_VERSION           0xD100
#define SPS30_PTR_READ_DEVICE_STATUS     0xD206
#define SPS30_PTR_CLEAR_DEVICE_STATUS    0xD210
#define SPS30_PTR_DEVICE_RESET           0xD304

/* Start Measurement output format bytes (datasheet §6.3.1) */
#define SPS30_MEAS_FLOAT   0x03
#define SPS30_MEAS_UINT16  0x05

/* CRC-8 parameters (datasheet §6.2) */
#define SPS30_CRC_POLY  0x31
#define SPS30_CRC_INIT  0xFF

/* Timing constants (datasheet §6.3) */
#define SPS30_CMD_DELAY_MS       20
#define SPS30_SLEEP_DELAY_MS     5
#define SPS30_WAKE_DELAY_MS      5
#define SPS30_FAN_SPIN_DOWN_MS   2000
#define SPS30_STARTUP_DELAY_MS   8000

/* Data-Ready flag values (datasheet §6.3.3) */
#define SPS30_DATA_NOT_READY 0x00
#define SPS30_DATA_READY     0x01

/* Device Status Register bit masks (datasheet §4.4) */
#define SPS30_STATUS_FAN_ERROR    BIT(4)
#define SPS30_STATUS_LASER_ERROR  BIT(5)
#define SPS30_STATUS_FAN_SPEED    BIT(21)

/* Number of float values in measurement response */
#define SPS30_NUM_VALUES 10

/* Size of Read Measured Values response in float mode (datasheet §4.3)
 * 10 values × (2 data bytes + 1 CRC) × 2 chunks = 60 bytes
 */
#define SPS30_FLOAT_RESPONSE_SIZE 60

struct sps30_config {
	struct i2c_dt_spec bus;
};

struct sps30_data {
	float pm1;    /* Mass Concentration PM1.0 [µg/m³] */
	float pm25;   /* Mass Concentration PM2.5 [µg/m³] */
	float pm4;    /* Mass Concentration PM4.0 [µg/m³] */
	float pm10;   /* Mass Concentration PM10  [µg/m³] */
	char product_type[32];
	char serial_number[32];
	uint8_t fw_major;
	uint8_t fw_minor;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SPS30_SPS30_INTERNAL_H_ */
