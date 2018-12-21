/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <i2s.h>

int _impl_i2s_configure(struct device *dev, enum i2s_dir dir,
			struct i2s_config *cfg)
{
	const struct i2s_driver_api *api = dev->driver_api;

	return api->configure(dev, dir, cfg);
}

int _impl_i2s_trigger(struct device *dev, enum i2s_dir dir,
		      enum i2s_trigger_cmd cmd)
{
	const struct i2s_driver_api *api = dev->driver_api;

	return api->trigger(dev, dir, cmd);
}
