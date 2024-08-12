/*
 * Copyright (c) 2024 Bittium Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMP435_H_
#define ZEPHYR_DRIVERS_SENSOR_TMP435_H_

#define TMP435_CONF_REG_1         0x03
#define TMP435_CONF_REG_1_DATA    0xc4
/* [7]=1 ALERT Masked, [6]=1 Shut Down (one shot mode), [2]=1 −55 C to +150 C */
#define TMP435_CONF_REG_2         0x1a
#define TMP435_CONF_REG_2_REN     0x10 /* [4]=1 External channel 1 enabled */
#define TMP435_CONF_REG_2_RC      0x04 /* [2]=1 Resistance correction enabled */
#define TMP435_CONF_REG_2_DATA    0x08 /* [3]=1 Local channel enabled */
#define TMP435_BETA_RANGE_REG     0x25
#define TMP435_STATUS_REG         0x02
#define TMP435_STATUS_REG_BUSY    0x80 /* conv not ready */
#define TMP435_SOFTWARE_RESET_REG 0xfc
#define TMP435_ONE_SHOT_START_REG 0x0f
#define TMP435_LOCAL_TEMP_H_REG   0x00
#define TMP435_LOCAL_TEMP_L_REG   0x15
#define TMP435_REMOTE_TEMP_H_REG  0x01
#define TMP435_REMOTE_TEMP_L_REG  0x10

#define TMP435_CONV_LOOP_LIMIT 50   /* max 50*10 ms */
#define TMP435_FRACTION_INC    0x80 /* 0.5000 */

static const int32_t tmp435_temp_offset = -64;

struct tmp435_data {
	int32_t temp_die;     /* Celsius degrees */
	int32_t temp_ambient; /* Celsius degrees */
};

struct tmp435_config {
	struct i2c_dt_spec i2c;
	bool external_channel;
	bool resistance_correction;
	uint8_t beta_compensation;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_TMP435_H_ */
