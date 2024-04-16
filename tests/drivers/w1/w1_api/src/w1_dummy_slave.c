/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_w1_device

#include <zephyr/device.h>
#include <zephyr/drivers/w1.h>

struct w1_dummy_slave_api {
};

static const struct w1_dummy_slave_api w1_dummy_slave_api1 = {};

#define TEST_W1_DUMMY_SLAVE_DEFINE(inst)                                                           \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, NULL, NULL, POST_KERNEL,                           \
			      CONFIG_W1_INIT_PRIORITY, &w1_dummy_slave_api1);

DT_INST_FOREACH_STATUS_OKAY(TEST_W1_DUMMY_SLAVE_DEFINE)
