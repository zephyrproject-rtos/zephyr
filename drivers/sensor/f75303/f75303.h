/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_F75303_F75303_H_
#define ZEPHYR_DRIVERS_SENSOR_F75303_F75303_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#define F75303_LOCAL_TEMP_H	0x00
#define F75303_REMOTE1_TEMP_H	0x01
#define F75303_REMOTE1_TEMP_L	0x10
#define F75303_REMOTE2_TEMP_H	0x23
#define F75303_REMOTE2_TEMP_L	0x24
#define F75303_LOCAL_TEMP_L	0x29

struct f75303_data {
	uint16_t sample_local;
	uint16_t sample_remote1;
	uint16_t sample_remote2;
};

struct f75303_config {
	struct i2c_dt_spec i2c;
};

#endif
