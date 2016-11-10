/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sensor.h>
#include <i2c.h>

#define SYS_LOG_DOMAIN "FXOS8700"
#define SYS_LOG_LEVEL CONFIG_FXOS8700_SYS_LOG_LEVEL
#include <misc/sys_log.h>

#define FXOS8700_REG_STATUS			0x00
#define FXOS8700_REG_OUTXMSB			0x01
#define FXOS8700_REG_INT_SOURCE			0x0c
#define FXOS8700_REG_WHOAMI			0x0d
#define FXOS8700_REG_XYZ_DATA_CFG		0x0e
#define FXOS8700_REG_CTRLREG1			0x2a
#define FXOS8700_REG_CTRLREG2			0x2b
#define FXOS8700_REG_CTRLREG3			0x2c
#define FXOS8700_REG_CTRLREG4			0x2d
#define FXOS8700_REG_CTRLREG5			0x2e
#define FXOS8700_REG_M_OUTXMSB			0x33
#define FXOS8700_REG_M_CTRLREG1			0x5b
#define FXOS8700_REG_M_CTRLREG2			0x5c

#define FXOS8700_XYZ_DATA_CFG_FS_MASK		0x03

#define FXOS8700_CTRLREG1_ACTIVE_MASK		0x01
#define FXOS8700_CTRLREG1_DR_MASK		(7 << 3)

#define FXOS8700_CTRLREG2_RST_MASK		0x40

#define FXOS8700_M_CTRLREG1_MODE_MASK		0x03

#define FXOS8700_M_CTRLREG2_AUTOINC_MASK	(1 << 5)

#define FXOS8700_NUM_ACCEL_CHANNELS		3
#define FXOS8700_NUM_MAG_CHANNELS		3
#define FXOS8700_NUM_HYBRID_CHANNELS		6
#define FXOS8700_MAX_NUM_CHANNELS		6

#define FXOS8700_BYTES_PER_CHANNEL_NORMAL	2
#define FXOS8700_BYTES_PER_CHANNEL_FAST		1

#define FXOS8700_MAX_NUM_BYTES		(FXOS8700_BYTES_PER_CHANNEL_NORMAL * \
					 FXOS8700_MAX_NUM_CHANNELS)

enum fxos8700_power {
	FXOS8700_POWER_STANDBY		= 0,
	FXOS8700_POWER_ACTIVE,
};

enum fxos8700_mode {
	FXOS8700_MODE_ACCEL		= 0,
	FXOS8700_MODE_MAGN		= 1,
	FXOS8700_MODE_HYBRID		= 3,
};

enum fxos8700_range {
	FXOS8700_RANGE_2G		= 0,
	FXOS8700_RANGE_4G,
	FXOS8700_RANGE_8G,
};

enum fxos8700_channel {
	FXOS8700_CHANNEL_ACCEL_X	= 0,
	FXOS8700_CHANNEL_ACCEL_Y,
	FXOS8700_CHANNEL_ACCEL_Z,
	FXOS8700_CHANNEL_MAGN_X,
	FXOS8700_CHANNEL_MAGN_Y,
	FXOS8700_CHANNEL_MAGN_Z,
};

struct fxos8700_config {
	char *i2c_name;
	uint8_t i2c_address;
	uint8_t whoami;
	enum fxos8700_mode mode;
	enum fxos8700_range range;
	uint8_t start_addr;
	uint8_t start_channel;
	uint8_t num_channels;
};

struct fxos8700_data {
	struct device *i2c;
	struct k_sem sem;
	int16_t raw[FXOS8700_MAX_NUM_CHANNELS];
};
