/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>


#define DUMMY_DRIVER_NAME	"dummy_driver"

typedef int (*dummy_api_configure_t)(const struct device *dev,
				     uint32_t dev_config);


struct dummy_driver_api {
	dummy_api_configure_t configure;
};

static int dummy_configure(const struct device *dev, uint32_t config)
{
	return 0;
}

static const struct dummy_driver_api funcs = {
	.configure = dummy_configure,
};

int dummy_init(const struct device *dev)
{
	return 0;
}

/**
 * @cond INTERNAL_HIDDEN
 */
DEVICE_DEFINE(dummy_driver, DUMMY_DRIVER_NAME, &dummy_init,
		device_pm_control_nop, NULL, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &funcs);

/**
 * @endcond
 */
