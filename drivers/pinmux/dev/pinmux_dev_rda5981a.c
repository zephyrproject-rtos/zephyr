/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <pinmux.h>
#include <pinmux/rda5981a/pinmux_rda5981a.h>

#include "pinmux.h"

static int pinmux_set(struct device *dev, uint32_t pin, uint32_t func)
{
	return pinmux_rda5981a_set(pin, func);
}

static int pinmux_get(struct device *dev, uint32_t pin, uint32_t *func)
{
	return -ENOTSUP;
}

static const struct pinmux_driver_api pinmux_api = {
	.set = pinmux_set,
	.get = pinmux_get,
};

static int pinmux_rda5981a_init(struct device *port)
{
	return 0;
}

DEVICE_AND_API_INIT(pmux_dev, CONFIG_PINMUX_DEV_NAME, &pinmux_rda5981a_init,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_PINMUX_INIT_PRIORITY,
		    &pinmux_api);
