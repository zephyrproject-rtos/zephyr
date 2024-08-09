/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_DRIVERS_SENSOR_ADXL34X_TEST_H_
#define ZEPHYR_TEST_DRIVERS_SENSOR_ADXL34X_TEST_H_

#include <stdint.h>
#include <stdbool.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#define Q31_SCALE               ((int64_t)INT32_MAX + 1)
#define FLOAT_TO_Q31(x, shift)  ((int64_t)((float)(x) * (float)Q31_SCALE) >> shift)
#define Q31_TO_FLOAT(x, shift)  ((float)((int64_t)(x) << shift) / (float)Q31_SCALE)
#define DOUBLE_TO_Q31(x, shift) ((int64_t)((double)(x) * (double)Q31_SCALE) >> shift)
#define Q31_TO_DOUBLE(x, shift) ((double)((int64_t)(x) << shift) / (double)Q31_SCALE)
#define G_TO_MS2(g)             (g * SENSOR_G / 1000000LL)
#define MS2_TO_G(ms)            (ms / SENSOR_G * 1000000LL)

enum ADXL34X_TEST {
	ADXL34X_TEST_SPI_0,
	ADXL34X_TEST_SPI_1,
	ADXL34X_TEST_SPI_2,
	ADXL34X_TEST_I2C_53,
	ADXL34X_TEST_I2C_54,
	ADXL34X_TEST_I2C_55,
	/* Keep this entry last */
	ADXL34X_TEST_NR_OF_DEVICES,
};

struct adxl34x_device {
	const struct device *dev;
	const struct emul *emul;
};

struct adxl34x_fixture {
	struct adxl34x_device device[ADXL34X_TEST_NR_OF_DEVICES];
};

bool is_equal_sensor_value(struct sensor_value value_1, struct sensor_value value_2);

void *adxl34x_suite_setup(void);
void adxl34x_suite_before(void *a_fixture);
void adxl34x_is_ready(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device);
void adxl34x_is_not_ready(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device);
const char *adxl34x_get_name(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device);
const char *adxl34x_get_bus_name(struct adxl34x_fixture *fixture,
				 const enum ADXL34X_TEST test_device);
bool is_equal_float(const float value_1, const float value_2, const float error);
bool is_equal_double(const double value_1, const double value_2, const double error);

#endif /* ZEPHYR_TEST_DRIVERS_SENSOR_ADXL34X_TEST_H_ */
