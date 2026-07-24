/* Copyright (c) 2026 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blocking_emul.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/devicetree.h>
#include <zephyr/drivers/i3c_emul.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT zephyr_blocking_emul_i3c

LOG_MODULE_REGISTER(blocking_emul_i3c, CONFIG_I3C_LOG_LEVEL);

DEFINE_FAKE_VALUE_FUNC(int, blocking_emul_i3c_xfers, const struct emul *, struct i3c_msg *,
		       uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, blocking_emul_i3c_do_ccc, const struct emul *,
		       struct i3c_ccc_payload *);

static const struct i3c_emul_api blocking_emul_api = {
	.xfers = blocking_emul_i3c_xfers,
	.do_ccc = blocking_emul_i3c_do_ccc,
};

struct blocking_emul_cfg {
	uint64_t pid;
	uint8_t bcr;
	uint8_t dcr;
};

static int blocking_emul_init(const struct emul *target, const struct device *parent)
{
	const struct blocking_emul_cfg *cfg = (const struct blocking_emul_cfg *)target->cfg;

	ARG_UNUSED(parent);

	target->bus.i3c->pid = cfg->pid;
	target->bus.i3c->bcr = cfg->bcr;
	target->bus.i3c->dcr = cfg->dcr;
	target->bus.i3c->enabled_events = I3C_CCC_EVT_INTR | I3C_CCC_EVT_CR;

	return 0;
}

#define BLOCKING_EMUL_INIT(n)                                                                      \
	static const struct blocking_emul_cfg blocking_emul_cfg_##n = {                            \
		.pid = ((uint64_t)DT_INST_PROP_BY_IDX(n, reg, 1) << 32) |                          \
		       DT_INST_PROP_BY_IDX(n, reg, 2),                                             \
		.bcr = DT_INST_PROP(n, bcr),                                                       \
		.dcr = DT_INST_PROP(n, dcr),                                                       \
	};                                                                                         \
	I3C_DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, &blocking_emul_cfg_##n, POST_KERNEL,        \
				  CONFIG_APPLICATION_INIT_PRIORITY, NULL);                         \
	EMUL_DT_INST_DEFINE(n, blocking_emul_init, NULL, &blocking_emul_cfg_##n, &blocking_emul_api, \
			    NULL);

DT_INST_FOREACH_STATUS_OKAY(BLOCKING_EMUL_INIT)
