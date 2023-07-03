/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mfd/npm1300.h>

struct mfd_npm1300_config {
	struct i2c_dt_spec i2c;
};

struct mfd_npm1300_data {
	struct k_mutex mutex;
};

static int mfd_npm1300_init(const struct device *dev)
{
	const struct mfd_npm1300_config *config = dev->config;
	struct mfd_npm1300_data *mfd_data = dev->data;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	k_mutex_init(&mfd_data->mutex);

	return 0;
}

int mfd_npm1300_reg_read_burst(const struct device *dev, uint8_t base, uint8_t offset, void *data,
			       size_t len)
{
	const struct mfd_npm1300_config *config = dev->config;
	uint8_t buff[] = {base, offset};

	return i2c_write_read_dt(&config->i2c, buff, sizeof(buff), data, len);
}

int mfd_npm1300_reg_read(const struct device *dev, uint8_t base, uint8_t offset, uint8_t *data)
{
	return mfd_npm1300_reg_read_burst(dev, base, offset, data, 1U);
}

int mfd_npm1300_reg_write(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data)
{
	const struct mfd_npm1300_config *config = dev->config;
	uint8_t buff[] = {base, offset, data};

	return i2c_write_dt(&config->i2c, buff, sizeof(buff));
}

int mfd_npm1300_reg_write2(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data1,
			   uint8_t data2)
{
	const struct mfd_npm1300_config *config = dev->config;
	uint8_t buff[] = {base, offset, data1, data2};

	return i2c_write_dt(&config->i2c, buff, sizeof(buff));
}

int mfd_npm1300_reg_update(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data,
			   uint8_t mask)
{
	struct mfd_npm1300_data *mfd_data = dev->data;
	uint8_t reg;
	int ret;

	k_mutex_lock(&mfd_data->mutex, K_FOREVER);

	ret = mfd_npm1300_reg_read(dev, base, offset, &reg);

	if (ret == 0) {
		reg = (reg & ~mask) | (data & mask);
		ret = mfd_npm1300_reg_write(dev, base, offset, reg);
	}

	k_mutex_unlock(&mfd_data->mutex);

	return ret;
}

#define MFD_NPM1300_DEFINE(inst)                                                                   \
	static struct mfd_npm1300_data data_##inst;                                                \
                                                                                                   \
	static const struct mfd_npm1300_config config##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_npm1300_init, NULL, &data_##inst, &config##inst,           \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_NPM1300_DEFINE)
