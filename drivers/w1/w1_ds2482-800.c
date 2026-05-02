/*
 * Copyright (c) 2023 Caspar Friedrich <c.s.w.friedrich@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "w1_ds2482-800.h"
#include "w1_ds2482_84_common.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT maxim_ds2482_800

LOG_MODULE_REGISTER(ds2482, CONFIG_W1_LOG_LEVEL);

struct ds2482_config {
	const struct i2c_dt_spec i2c_spec;
};

struct ds2482_data {
	struct k_mutex lock;
};

int ds2482_change_bus_lock_impl(const struct device *dev, bool lock)
{
	struct ds2482_data *data = dev->data;

	return lock ? k_mutex_lock(&data->lock, K_FOREVER) : k_mutex_unlock(&data->lock);
}

static int ds2482_init(const struct device *dev)
{
	int ret;

	const struct ds2482_config *config = dev->config;
	struct ds2482_data *data = dev->data;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c_spec)) {
		return -ENODEV;
	}

	ret = ds2482_84_reset_device(&config->i2c_spec);
	if (ret < 0) {
		LOG_ERR("Device reset failed: %d", ret);
		return ret;
	}

	return 0;
}

#define DS2482_INIT(inst)                                                                          \
	static const struct ds2482_config inst_##inst##_config = {                                 \
		.i2c_spec = I2C_DT_SPEC_INST_GET(inst),                                            \
	};                                                                                         \
	static struct ds2482_data inst_##inst##_data;                                              \
	DEVICE_DT_INST_DEFINE(inst, ds2482_init, NULL, &inst_##inst##_data, &inst_##inst##_config, \
			      POST_KERNEL, CONFIG_W1_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DS2482_INIT)

/*
 * Make sure that this driver is not initialized before the i2c bus is available
 */
BUILD_ASSERT(CONFIG_W1_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY);
