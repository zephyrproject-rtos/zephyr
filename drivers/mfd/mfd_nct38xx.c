/*
 * Copyright (c) 2023 Google, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct38xx

#include <zephyr/drivers/i2c.h>

struct mfd_nct38xx_config {
	const struct i2c_dt_spec i2c_dev;
};

struct mfd_nct38xx_data {
	/* lock NC38xx register access */
	struct k_sem lock;
};

static int mfd_nct38xx_init(const struct device *dev)
{
	const struct mfd_nct38xx_config *config = dev->config;
	struct mfd_nct38xx_data *data = dev->data;

	if (!device_is_ready(config->i2c_dev.bus)) {
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	return 0;
}

struct k_sem *mfd_nct38xx_get_lock_reference(const struct device *dev)
{
	struct mfd_nct38xx_data *data = dev->data;

	return &data->lock;
}

const struct i2c_dt_spec *mfd_nct38xx_get_i2c_dt_spec(const struct device *dev)
{
	const struct mfd_nct38xx_config *config = dev->config;

	return &config->i2c_dev;
}

#define MFD_NCT38XX_DEFINE(inst)                                                                   \
	static struct mfd_nct38xx_data nct38xx_data_##inst;                                        \
	static const struct mfd_nct38xx_config nct38xx_cfg_##inst = {                              \
		.i2c_dev = I2C_DT_SPEC_INST_GET(inst),                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_nct38xx_init, NULL, &nct38xx_data_##inst,                  \
			      &nct38xx_cfg_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_NCT38XX_DEFINE)
