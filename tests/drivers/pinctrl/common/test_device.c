/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_pinctrl_device

#include "test_device.h"

#include <device.h>
#include <drivers/pinctrl.h>

int test_device_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

#if !defined(CONFIG_PINCTRL_TEST_STORE_CUSTOM_REG)
#define TEST_PINCTRL_CONFIG(inst)					\
			PINCTRL_DT_INST_DEFINE(inst)
#else
#define TEST_PINCTRL_CONFIG(inst)					\
			PINCTRL_DT_INST_CUSTOM_REG_DEFINE(inst,		\
					DT_REG_ADDR(DT_DRV_INST(index))))
#endif

#define PINCTRL_DEVICE_INIT(inst)					\
	PINCTRL_DT_INST_DEFINE(inst)					\
									\
	DEVICE_DT_INST_DEFINE(inst, test_device_init, NULL, NULL, NULL,	\
			      POST_KERNEL,				\
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_DEVICE_INIT)
