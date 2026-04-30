/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Opamp driver for Infineon AutAnalog CTB.
 *
 * Each instance represents one of the two opamps (OA0, OA1) within
 * a CTB block.  The driver implements the Zephyr opamp API.
 *
 * Gain is configured at build time via devicetree only.  Runtime gain
 * changes are not supported because the Autonomous Controller
 * start/pause cycle required to reload the State Transition Table
 * disrupts the shared SAR ADC sequencer.
 */

#define DT_DRV_COMPAT infineon_autanalog_ctb_opamp

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/opamp.h>
#include <infineon_autanalog_ctb.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(opamp_ifx_autanalog_ctb, CONFIG_OPAMP_LOG_LEVEL);

struct opamp_ifx_ctb_config {
	const struct device *ctb_mfd;
	uint8_t oa_idx;
	uint8_t topology;
};

static int opamp_ifx_ctb_set_gain(const struct device *dev, enum opamp_gain gain)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(gain);

	return -ENOTSUP;
}

static DEVICE_API(opamp, opamp_ifx_ctb_api) = {
	.set_gain = opamp_ifx_ctb_set_gain,
};

static int opamp_ifx_ctb_init(const struct device *dev)
{
	const struct opamp_ifx_ctb_config *cfg = dev->config;

	if (!device_is_ready(cfg->ctb_mfd)) {
		LOG_ERR("CTB MFD device not ready");
		return -ENODEV;
	}

	LOG_DBG("CTB OA%u initialized (topology %u)", cfg->oa_idx, cfg->topology);
	return 0;
}

/* clang-format off */

#define OPAMP_IFX_CTB_DEFINE(n)                                                                \
	static const struct opamp_ifx_ctb_config opamp_ifx_ctb_config_##n = {                      \
		.ctb_mfd = DEVICE_DT_GET(DT_INST_PARENT(n)),                                       \
		.oa_idx = (uint8_t)DT_INST_REG_ADDR(n),                                            \
		.topology = (uint8_t)DT_INST_PROP(n, topology),                                    \
	};                                                                                         \
                                                                                               \
	DEVICE_DT_INST_DEFINE(n, &opamp_ifx_ctb_init, NULL, NULL, &opamp_ifx_ctb_config_##n,       \
			      POST_KERNEL, CONFIG_OPAMP_INFINEON_AUTANALOG_CTB_INIT_PRIORITY,      \
			      &opamp_ifx_ctb_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(OPAMP_IFX_CTB_DEFINE)
