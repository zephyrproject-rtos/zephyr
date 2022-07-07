/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_pinctrl_device

#include "test_device.h"

#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>

int test_device_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#define PINCTRL_DEVICE_INIT(inst)					\
	PINCTRL_DT_INST_DEFINE(inst);					\
									\
	DEVICE_DT_INST_DEFINE(inst, test_device_init, NULL, NULL, NULL,	\
			      POST_KERNEL,				\
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_DEVICE_INIT)
