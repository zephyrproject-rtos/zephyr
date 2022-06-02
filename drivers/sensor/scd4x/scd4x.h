/*
 * Copyright (c) 2022 Stephen Oliver
 *
 * SPDX-License-Identifier: Apache-2.0
 */   

#ifndef ZEPHYR_DRIVERS_SENSOR_SCD4X_SCD4X_H_
#define ZEPHYR_DRIVERS_SENSOR_SCD4X_SCD4X_H_

#include <zephyr/device.h>


#define SCD4X_MAX_AMBIENT_PRESSURE UINT16_MAX

#define SCD4X_CMD_POWER_DOWN 	0x36E0
#define SCD4X_CMD_WAKE_UP		0x36F6
#define SCD4X_CMD_REINIT		0x3646

#define SCD4X_CMD_START_PERIODIC_MEASUREMENT 			0x21B1
#define SCD4X_CMD_STOP_PERIODIC_MEASUREMENT 			0x3F86
#define SCD4X_CMD_START_LOW_POWER_PERIODIC_MEASUREMENT 	0x21AC

#define SCD4X_CMD_GET_DATA_READY_STATUS 0xE4B8
#define SCD4X_CMD_READ_MEASUREMENT 		0xEC05

#define SCD4X_CMD_PERSIST_SETTINGS 	0x3615

#define SCD4X_CMD_GET_SERIAL_NUMBER 0x3682

#define SCD4X_CMD_PERFORM_SELF_TEST 0x3639
#define SCD4X_CMD_PERFORM_FACTORY_RESET 0x3632

#define SCD4X_CMD_SET_TEMPERATURE_OFFSET 0x241D
#define SCD4X_CMD_GET_TEMPERATURE_OFFSET 0x2318

#define SCD4X_CMD_SET_SENSOR_ALTITUDE 0x2427
#define SCD4X_CMD_GET_SENSOR_ALTITUDE 0x2322

#define SCD4X_CMD_SET_AMBIENT_PRESSURE 0xE000

#define SCD4X_CMD_PERFORM_FORCED_RECALIBRATION 0x362F

#define SCD4X_CMD_SET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x2416
#define SCD4X_CMD_GET_AUTOMATIC_SELF_CALIBRATION_ENABLED 0x2313

#define SCD4X_POWER_DOWN_WAIT_MS	1
#define SCD4X_WAKE_UP_WAIT_MS	20
#define SCD4X_REINIT_WAIT_MS	20
#define SCD4X_PERFORM_SELF_TEST_WAIT_MS 10000
#define SCD4X_PERFORM_FACTORY_RESET_WAIT_MS 1200
#define SCD4X_STOP_PERIODIC_MEASUREMENT_WAIT_MS	500
#define SCD4X_READ_MEASUREMENT_WAIT_MS	1
#define SCD4X_SET_TEMPERATURE_OFFSET_WAIT_MS	1
#define SCD4X_GET_TEMPERATURE_OFFSET_WAIT_MS	1
#define SCD4X_SET_SENSOR_ALTITUDE_WAIT_MS	1
#define SCD4X_GET_SENSOR_ALTITUDE_WAIT_MS	1
#define SCD4X_SET_AMBIENT_PRESSURE_WAIT_MS	1
#define SCD4X_SET_AUTOMATIC_CALIBRATION_WAIT_MS	1


#define SCD4X_CMD_MEASURE_SINGLE_SHOT 0x219D
#define SCD4X_CMD_MEASURE_SINGLE_SHOT_RHT_ONLY 0x2196

#define SCD4X_MEASURE_SINGLE_SHOT_WAIT_MS 5000
#define SCD4X_MEASURE_SINGLE_SHOT_RHT_ONLY_WAIT_MS 50

/*
* Used to mask SCD4X_CMD_GET_DATA_READY_STATUS response value.

* The sensor datasheet does not document the meaning of each bit, nor does it state that any
* particular bit will be set to 1 when data is ready, it only guarantees that the device is
* NOT ready if these bits are all 0, and that any other value means data is ready.
*/
#define SCD4X_MEASURE_READY(x) (((x) & 0x07FF) != 0)

/*
 * CRC parameters from SCD4X datasheet version 1.2, section 3.11
 */
#define SCD4X_CRC_POLY		0x31
#define SCD4X_CRC_INIT		0xFF

/* defines matching the related enums DT_ENUM_IDX: */
#define MODEL_SCD40	0
#define MODEL_SCD41	1
#define MEASURE_MODE_NORMAL			0
#define MEASURE_MODE_LOW_POWER		1
#define MEASURE_MODE_SINGLE_SHOT	2

enum scd4x_model {
	SCD40 = MODEL_SCD40,
	SCD41 = MODEL_SCD41,
};

enum scd4x_measure_mode {
	NORMAL = MEASURE_MODE_NORMAL,
	LOW_POWER = MEASURE_MODE_LOW_POWER,
	SINGLE_SHOT = MEASURE_MODE_SINGLE_SHOT,
};

struct scd4x_config {
	struct i2c_dt_spec bus;
	enum scd4x_model model;
	enum scd4x_measure_mode measure_mode;
	bool auto_calibration;
	uint16_t temperature_offset;
	uint16_t altitude;
};

struct scd4x_data {
	uint16_t t_sample;
	uint16_t rh_sample;
	uint16_t co2_sample;
	char serial_number[15];
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SCD4X_SCD4X_H_ */
