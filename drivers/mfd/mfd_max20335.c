/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max20335

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>

#define MAX20335_REG_CHIP_ID 0x00
#define MAX20335_CHIP_ID_VAL 0x04

struct mfd_max20335_config {
	struct i2c_dt_spec bus;
};

static int mfd_max20335_init(const struct device *dev)
{
	const struct mfd_max20335_config *config = dev->config;
	uint8_t val;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_CHIP_ID, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != MAX20335_CHIP_ID_VAL) {
		return -ENODEV;
	}

	return 0;
}

#define MFD_MA20335_DEFINE(inst)					\
	static const struct mfd_max20335_config mfd_max20335_config##inst = { \
		.bus = I2C_DT_SPEC_INST_GET(inst),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst, mfd_max20335_init, NULL, NULL,	\
			      &mfd_max20335_config##inst, POST_KERNEL,	\
			      CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_MA20335_DEFINE)
