/* Copyright (c) 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blocking_emul.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT zephyr_blocking_emul

LOG_MODULE_REGISTER(blocking_emul, CONFIG_I2C_LOG_LEVEL);

DEFINE_FAKE_VALUE_FUNC(int, blocking_emul_i2c_transfer, const struct emul *, struct i2c_msg *, int,
		       int);

static struct i2c_emul_api blocking_emul_i2c_api = {
	.transfer = blocking_emul_i2c_transfer,
};

static int blocking_emul_init(const struct emul *target, const struct device *parent)
{
	return 0;
}

static int blocking_dev_init(const struct device *dev)
{
	return 0;
}

#define DECLARE_DRV(n)                                                                             \
	EMUL_DT_INST_DEFINE(n, blocking_emul_init, NULL, NULL, &blocking_emul_i2c_api, NULL);      \
	DEVICE_DT_INST_DEFINE(n, blocking_dev_init, NULL, NULL, NULL, POST_KERNEL, 99, NULL);

DT_INST_FOREACH_STATUS_OKAY(DECLARE_DRV)
