/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fsl_opamp.h>
#include <zephyr/drivers/opamp.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_opamp_fast, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_opamp_fast

struct mcux_opamp_fast_config {
	OPAMP_Type *base;
	opamp_bias_current_t bias_current;
	uint8_t functional_mode;
	uint8_t compensation_capcitor;
	bool is_reference_source;
};

static int mcux_opamp_fast_enable(const struct device *dev)
{
	const struct mcux_opamp_fast_config *config = dev->config;
	OPAMP_Type *base = config->base;

	OPAMP_Enable(base, true);

	return 0;
}

static int mcux_opamp_fast_disable(const struct device *dev)
{
	const struct mcux_opamp_fast_config *config = dev->config;
	OPAMP_Type *base = config->base;

	OPAMP_Enable(base, false);

	return 0;
}

static DEVICE_API(opamp, api) = {
	.enable = mcux_opamp_fast_enable,
	.disable = mcux_opamp_fast_disable,
};

int mcux_opamp_fast_init(const struct device *dev)
{
	const struct mcux_opamp_fast_config *config = dev->config;
	OPAMP_Type *base = config->base;

	OPAMP_SetBiasCurrent(base, config->bias_current);
	OPAMP_SetCompensationCapcitor(base, config->compensation_capcitor);

	if (config->is_reference_source) {
		OPAMP_Enable(base, true);
	}

	return 0;
}

#define OPAMP_MCUX_OPAMP_FAST_DEFINE(inst)					\
	static const struct mcux_opamp_fast_config _CONCAT(config, inst) = {	\
		.base = (OPAMP_Type *)DT_INST_REG_ADDR(inst),			\
		.bias_current = DT_INST_ENUM_IDX(inst, bias_current),		\
		.compensation_capcitor = DT_INST_ENUM_IDX(inst,			\
				compensation_capcitor),				\
		.functional_mode = DT_INST_ENUM_IDX(inst, functional_mode),	\
		.is_reference_source = DT_INST_PROP(inst, is_reference_source),	\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, mcux_opamp_fast_init, NULL, NULL,		\
				&_CONCAT(config, inst), POST_KERNEL,		\
				CONFIG_OPAMP_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(OPAMP_MCUX_OPAMP_FAST_DEFINE)
