/*
 * Copyright (c) 2023 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for Bosch BMI270 sensor
 *
 * This exposes an API to the sensor's CRT feature which
 * is specific to the application/environment and cannot
 * be expressed within the sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMI270_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMI270_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/* Sensor specific attributes */
enum sensor_attribute_bmi270 {
	/* Programs the non volatile memory(nvm) of bmi270 */
	BMI270_SENSOR_ATTR_NVM_PROG = SENSOR_ATTR_PRIV_START,
	/* Set BMI270 user gain compensation */
	BMI270_SENSOR_ATTR_GAIN_COMP
};

/* Macros for setting gyro gain compensation */
#define BMI270_GYR_GAIN_EN                 0x01
#define BMI270_GYR_GAIN_DIS                0x00

/* Helper Macros for setting gyro gain compensation */
#define BMI270_SENSOR_VALUE_WITH_GAIN(var_name, gain_value) \
	struct sensor_value var_name = {.val1 = gain_value, .val2 = 0}

#define BMI270_SENSOR_ATTR_SET_GAIN_COMPENSATION(dev, gain_value) \
	do { \
		BMI270_SENSOR_VALUE_WITH_GAIN(_gain_compensation_var, gain_value); \
		sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, \
			BMI270_SENSOR_ATTR_GAIN_COMP, &_gain_compensation_var); \
	} while (0)

/** @name Enum to define status of gyroscope trigger G_TRIGGER */
typedef enum {
	/*Command is valid*/
	BMI270_CRT_TRIGGER_STAT_SUCCESS = 0x00,
	/*Command is aborted due to incomplete pre-conditions*/
	BMI270_CRT_TRIGGER_STAT_PRECON_ERR = 0x01,
	/*Command is aborted due to unsuccessful download*/
	BMI270_CRT_TRIGGER_STAT_DL_ERR = 0x02,
	/*Command is aborted by host or due to motion detection*/
	BMI270_CRT_TRIGGER_STAT_ABORT_ERR = 0x03
} gtrigger_status;

/** @name Structure to define gyroscope saturation status of user gain
 * Describes the saturation status for the gyroscope gain
 * update and G_TRIGGER command status
 */
struct bmi2_gyr_user_gain_status {
	/* Status in x-axis
	 * This bit will be 1 if the updated gain
	 * results to saturated value based on the
	 * ratio provided for x axis, otherwise it will be 0
	 */
	uint8_t sat_x;

	/* Status in y-axis
	 * This bit will be 1 if the updated gain
	 * results to saturated value based on the
	 * ratio provided for y axis, otherwise it will be 0
	 */
	uint8_t sat_y;

	/* Status in z-axis
	 * This bit will be 1 if the updated gain
	 * results to saturated value based on the
	 * ratio provided for z axis, otherwise it will be 0
	 */
	uint8_t sat_z;

	/* G trigger status
	 * Status of gyroscope trigger G_TRIGGER command.
	 * These bits are updated at the end of feature execution .
	 */
	gtrigger_status g_trigger_status;
};

/** @name Structure to store the compensated user-gain data of gyroscope */
struct bmi2_gyro_user_gain_data {
	/** x-axis */
	int8_t x;

	/** y-axis */
	int8_t y;

	/** z-axis */
	int8_t z;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_BMI270_H_ */
