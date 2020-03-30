/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>


#define BAD_DRIVER_NAME	"bad_driver"

typedef int (*bad_api_configure_t)(struct device *dev,
				     u32_t dev_config);


struct bad_driver_api {
	bad_api_configure_t configure;
};

static int bad_configure(struct device *dev, u32_t config)
{
	return 0;
}

static const struct bad_driver_api funcs = {
	.configure = bad_configure,
};

int bad_driver_init(struct device *dev)
{
	return -EINVAL;
}

/**
 * @cond INTERNAL_HIDDEN
 */
DEVICE_AND_API_INIT(bad_driver, BAD_DRIVER_NAME, &bad_driver_init,
		    NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &funcs);

/**
 * @endcond
 */
