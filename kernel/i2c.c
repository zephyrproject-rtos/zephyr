/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <i2c.h>

int _impl_i2c_configure(struct device *dev, u32_t dev_config)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->driver_api;

	return api->configure(dev, dev_config);
}

int _impl_i2c_transfer(struct device *dev,
		       struct i2c_msg *msgs, u8_t num_msgs,
		       u16_t addr)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->driver_api;

	return api->transfer(dev, msgs, num_msgs, addr);
}

int _impl_i2c_slave_register(struct device *dev,
			     struct i2c_slave_config *cfg)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->driver_api;

	if (!api->slave_register) {
		return -ENOTSUP;
	}

	return api->slave_register(dev, cfg);
}

int _impl_i2c_slave_unregister(struct device *dev,
			       struct i2c_slave_config *cfg)
{
	const struct i2c_driver_api *api =
		(const struct i2c_driver_api *)dev->driver_api;

	if (!api->slave_unregister) {
		return -ENOTSUP;
	}

	return api->slave_unregister(dev, cfg);
}

int _impl_i2c_slave_driver_register(struct device *dev)
{
	const struct i2c_slave_driver_api *api =
		(const struct i2c_slave_driver_api *)dev->driver_api;

	return api->driver_register(dev);
}

int _impl_i2c_slave_driver_unregister(struct device *dev)
{
	const struct i2c_slave_driver_api *api =
		(const struct i2c_slave_driver_api *)dev->driver_api;

	return api->driver_unregister(dev);
}
