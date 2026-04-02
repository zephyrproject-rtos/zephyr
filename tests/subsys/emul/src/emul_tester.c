/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "emul_tester.h"

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_stub_device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

#define DT_DRV_COMPAT vnd_emul_tester

struct emul_tester_cfg {
	int scale;
};

struct emul_tester_data {
	int action;
};

static int emul_tester_set_action(const struct emul *target, int action)
{
	const struct emul_tester_cfg *cfg = target->cfg;
	struct emul_tester_data *data = target->data;

	data->action = action * cfg->scale;

	return 0;
}

static int emul_tester_get_action(const struct emul *target, int *action)
{
	struct emul_tester_data *data = target->data;

	*action = data->action;

	return 0;
}

static int emul_tester_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				int addr)
{
	ARG_UNUSED(target);
	ARG_UNUSED(msgs);
	ARG_UNUSED(num_msgs);
	ARG_UNUSED(addr);

	return -ENOTSUP;
}

static struct i2c_emul_api bus_api = {
	.transfer = emul_tester_transfer,
};

static const struct emul_tester_backend_api emul_tester_backend_api = {
	.set_action = emul_tester_set_action,
	.get_action = emul_tester_get_action,
};

static int emul_tester_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

#define EMUL_TESTER(n)                                                                             \
	static struct emul_tester_data emul_tester_data_##n;                                       \
	static const struct emul_tester_cfg emul_tester_cfg_##n = {                                \
		.scale = DT_INST_PROP(n, scale),                                                   \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_tester_init, &emul_tester_data_##n, &emul_tester_cfg_##n,      \
			    &bus_api, &emul_tester_backend_api);

DT_INST_FOREACH_STATUS_OKAY(EMUL_TESTER)

DT_INST_FOREACH_STATUS_OKAY(EMUL_STUB_DEVICE)
