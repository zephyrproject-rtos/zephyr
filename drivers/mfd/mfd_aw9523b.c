/*
 * Copyright (c) 2024 TOKITA Hiroshi
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT awinic_aw9523b

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/aw9523b.h>
#include <zephyr/kernel.h>

#define AW9523B_ID_VALUE 0x23

struct mfd_aw9523b_config {
	struct i2c_dt_spec i2c;
};

struct mfd_aw9523b_data {
	struct k_sem lock;
};

static int mfd_aw9523b_init(const struct device *dev)
{
	const struct mfd_aw9523b_config *config = dev->config;
	struct mfd_aw9523b_data *data = dev->data;
	uint8_t reg;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	ret = i2c_reg_read_byte_dt(&config->i2c, AW9523B_REG_ID, &reg);

	if (ret) {
		return ret;
	};

	if (reg != AW9523B_ID_VALUE) {
		return -1;
	}

	return 0;
}

struct k_sem *aw9523b_get_lock(const struct device *dev)
{
	struct mfd_aw9523b_data *data = dev->data;

	return &data->lock;
}

#define MFD_AW9523B_DEFINE(inst)                                                                   \
	static const struct mfd_aw9523b_config config##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct mfd_aw9523b_data data##inst;                                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_aw9523b_init, NULL, &data##inst, &config##inst,            \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_AW9523B_DEFINE)
