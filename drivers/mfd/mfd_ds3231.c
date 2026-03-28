/*
 * Copyright (c) 2024 Gergo Vari <work@gergovari.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/mfd/ds3231.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mfd_ds3231, CONFIG_MFD_LOG_LEVEL);

#define DT_DRV_COMPAT maxim_ds3231_mfd

struct mfd_ds3231_data {
	struct k_sem lock;
	const struct device *dev;
};

struct mfd_ds3231_conf {
	struct i2c_dt_spec i2c_bus;
};

int mfd_ds3231_i2c_get_registers(const struct device *dev, uint8_t start_reg, uint8_t *buf,
				 const size_t buf_size)
{
	struct mfd_ds3231_data *data = dev->data;
	const struct mfd_ds3231_conf *config = dev->config;

	/* FIXME: bad start_reg/buf_size values break i2c for that run */

	(void)k_sem_take(&data->lock, K_FOREVER);
	int err = i2c_burst_read_dt(&config->i2c_bus, start_reg, buf, buf_size);

	k_sem_give(&data->lock);

	return err;
}

int mfd_ds3231_i2c_set_registers(const struct device *dev, uint8_t start_reg, const uint8_t *buf,
				 const size_t buf_size)
{
	struct mfd_ds3231_data *data = dev->data;
	const struct mfd_ds3231_conf *config = dev->config;

	(void)k_sem_take(&data->lock, K_FOREVER);
	int err = i2c_burst_write_dt(&config->i2c_bus, start_reg, buf, buf_size);

	k_sem_give(&data->lock);

	return err;
}

static int mfd_ds3231_init(const struct device *dev)
{
	struct mfd_ds3231_data *data = dev->data;
	const struct mfd_ds3231_conf *config = (struct mfd_ds3231_conf *)(dev->config);

	k_sem_init(&data->lock, 1, 1);
	if (!i2c_is_ready_dt(&(config->i2c_bus))) {
		LOG_ERR("I2C bus not ready.");
		return -ENODEV;
	}
	return 0;
}

#define MFD_DS3231_DEFINE(inst)                                                                    \
	static const struct mfd_ds3231_conf config##inst = {.i2c_bus =                             \
								    I2C_DT_SPEC_INST_GET(inst)};   \
	static struct mfd_ds3231_data data##inst;                                                  \
	DEVICE_DT_INST_DEFINE(inst, &mfd_ds3231_init, NULL, &data##inst, &config##inst,            \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_DS3231_DEFINE)
