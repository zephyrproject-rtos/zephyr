/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_I2C_I2C_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_I2C_I2C_H_

#define I2C_BITRATE_STANDARD	100000	/* 100 Kbit/s */
#define I2C_BITRATE_FAST	400000	/* 400 Kbit/s */
#define I2C_BITRATE_FAST_PLUS	1000000 /* 1 Mbit/s */
#define I2C_BITRATE_HIGH	3400000	/* 3.4 Mbit/s */
#define I2C_BITRATE_ULTRA	5000000 /* 5 Mbit/s */

#define SMB_CHANNEL_A	0
#define SMB_CHANNEL_B	1
#define SMB_CHANNEL_C	2
#define I2C_CHANNEL_D	3
#define I2C_CHANNEL_E	4
#define I2C_CHANNEL_F	5

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_I2C_I2C_H_ */
