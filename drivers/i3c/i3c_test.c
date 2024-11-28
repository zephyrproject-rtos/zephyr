/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This is not a real I3C driver. It is used to instantiate struct
 * devices for the "vnd,i3c" devicetree compatible used in test code.
 */

#define DT_DRV_COMPAT vnd_i3c

#include <zephyr/drivers/i3c.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

static int vnd_i3c_configure(const struct device *dev,
			     enum i3c_config_type type, void *config)
{
	return -ENOTSUP;
}

static int vnd_i3c_config_get(const struct device *dev,
			      enum i3c_config_type type, void *config)
{
	return -ENOTSUP;
}

static int vnd_i3c_recover_bus(const struct device *dev)
{
	return -ENOTSUP;
}

static DEVICE_API(i3c, vnd_i3c_api) = {
	.configure = vnd_i3c_configure,
	.config_get = vnd_i3c_config_get,
	.recover_bus = vnd_i3c_recover_bus,
};

#define VND_I3C_INIT(n)						  \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL,	  \
			      POST_KERNEL,			  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &vnd_i3c_api);

DT_INST_FOREACH_STATUS_OKAY(VND_I3C_INIT)
