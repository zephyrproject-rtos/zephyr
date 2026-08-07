/*
 * Copyright (c) 2024 Jan Fäh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SCD4X_SCD4X_H_
#define ZEPHYR_DRIVERS_SENSOR_SCD4X_SCD4X_H_

#include <zephyr/device.h>

#define SCD4X_CMD_REINIT                         0
#define SCD4X_CMD_START_PERIODIC_MEASUREMENT     1
#define SCD4X_CMD_STOP_PERIODIC_MEASUREMENT      2
#define SCD4X_CMD_READ_MEASUREMENT               3
#define SCD4X_CMD_SET_TEMPERATURE_OFFSET         4
#define SCD4X_CMD_GET_TEMPERATURE_OFFSET         5
#define SCD4X_CMD_SET_SENSOR_ALTITUDE            6
#define SCD4X_CMD_GET_SENSOR_ALTITUDE            7
#define SCD4X_CMD_SET_AMBIENT_PRESSURE           8
#define SCD4X_CMD_GET_AMBIENT_PRESSURE           9
#define SCD4X_CMD_FORCED_RECALIB                 10
#define SCD4X_CMD_SET_AUTOMATIC_CALIB_ENABLE     11
#define SCD4X_CMD_GET_AUTOMATIC_CALIB_ENABLE     12
#define SCD4X_CMD_LOW_POWER_PERIODIC_MEASUREMENT 13
#define SCD4X_CMD_GET_DATA_READY_STATUS          14
#define SCD4X_CMD_PERSIST_SETTINGS               15
#define SCD4X_CMD_SELF_TEST                      16
#define SCD4X_CMD_FACTORY_RESET                  17
#define SCD4X_CMD_MEASURE_SINGLE_SHOT            18
#define SCD4X_CMD_MEASURE_SINGLE_SHOT_RHT        19
#define SCD4X_CMD_POWER_DOWN                     20
#define SCD4X_CMD_WAKE_UP                        21
#define SCD4X_CMD_SET_SELF_CALIB_INITIAL_PERIOD  22
#define SCD4X_CMD_GET_SELF_CALIB_INITIAL_PERIOD  23
#define SCD4X_CMD_SET_SELF_CALIB_STANDARD_PERIOD 24
#define SCD4X_CMD_GET_SELF_CALIB_STANDARD_PERIOD 25

#define SCD4X_CRC_POLY 0x31
#define SCD4X_CRC_INIT 0xFF

#define SCD4X_STARTUP_TIME_MS 30

#define SCD4X_TEMPERATURE_OFFSET_IDX_MAX 20
#define SCD4X_SENSOR_ALTITUDE_IDX_MAX    3000
#define SCD4X_AMBIENT_PRESSURE_IDX_MAX   1200
#define SCD4X_BOOL_IDX_MAX               1

#define SCD4X_MAX_TEMP 175
#define SCD4X_MIN_TEMP -45

enum scd4x_model_t {
	SCD4X_MODEL_SCD40,
	SCD4X_MODEL_SCD41,
};

enum scd4x_mode_t {
	SCD4X_MODE_NORMAL,
	SCD4X_MODE_LOW_POWER,
	SCD4X_MODE_SINGLE_SHOT,
};

struct scd4x_config {
	struct i2c_dt_spec bus;
	enum scd4x_model_t model;
	enum scd4x_mode_t mode;
};

struct scd4x_data {
	uint16_t temp_sample;
	uint16_t humi_sample;
	uint16_t co2_sample;
};

struct cmds_t {
	uint16_t cmd;
	uint16_t cmd_duration_ms;
};

const struct cmds_t scd4x_cmds[] = {
	{0x3646, 30},   {0x21B1, 0},  {0x3F86, 500}, {0xEC05, 1},   {0x241D, 1},     {0x2318, 1},
	{0x2427, 1},    {0x2322, 1},  {0xE000, 1},   {0xE000, 1},   {0x362F, 400},   {0x2416, 1},
	{0x2313, 1},    {0x21AC, 0},  {0xE4B8, 1},   {0x3615, 800}, {0x3639, 10000}, {0x3632, 1200},
	{0x219D, 5000}, {0x2196, 50}, {0x36E0, 1},   {0x36F6, 30},  {0x2445, 1},     {0x2340, 1},
	{0x244E, 1},    {0x234B, 1},
};

#endif /* ZEPHYR_DRIVERS_SENSOR_SCD4X_SCD4X_H_ */
