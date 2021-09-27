/*
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>

#define DT_DRV_COMPAT	test_device_wakeup_state

typedef int (*dummy_api_configure_t)(const struct device *dev,
				     uint32_t dev_config);


struct dummy_driver_api {
	dummy_api_configure_t configure;
};

static int dummy_configure(const struct device *dev, uint32_t config)
{
	return 0;
}

static const struct dummy_driver_api dummy_driver_api = {
	.configure = dummy_configure,
};

#define DUMMY_DRIVER_INIT(n)						\
	static int dummy_##n##_init(const struct device *dev);		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      dummy_##n##_init,				\
			      NULL,					\
			      NULL,					\
			      NULL,					\
			      POST_KERNEL,				\
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			      &dummy_driver_api);			\
									\
	static int dummy_##n##_init(const struct device *dev)		\
	{								\
		return 0;						\
	}

DT_INST_FOREACH_STATUS_OKAY(DUMMY_DRIVER_INIT)
