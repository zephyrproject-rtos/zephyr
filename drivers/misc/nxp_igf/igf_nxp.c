/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_igf

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/misc/nxp_igf.h>
#include <zephyr/sys/util.h>

#include <fsl_igf.h>

#define IGF_CHANNEL_MAX 32U

/* One IGF channel's static configuration, resolved from devicetree. */
struct igf_nxp_channel_config {
	uint8_t channel;
	igf_config_t cfg;
};

struct igf_nxp_config {
	IGF_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct igf_nxp_channel_config *channels;
	uint8_t num_channels;
};

int nxp_igf_get_status(const struct device *dev, uint32_t channel, uint32_t *flags)
{
	const struct igf_nxp_config *config = dev->config;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if ((channel >= IGF_CHANNEL_MAX) || (flags == NULL)) {
		return -EINVAL;
	}

	*flags = IGF_GetStatusFlags(config->base, channel);

	return 0;
}

int nxp_igf_clear_status(const struct device *dev, uint32_t channel, uint32_t mask)
{
	const struct igf_nxp_config *config = dev->config;

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (channel >= IGF_CHANNEL_MAX) {
		return -EINVAL;
	}

	IGF_ClearStatusFlags(config->base, channel, mask);

	return 0;
}

static int igf_nxp_init(const struct device *dev)
{
	const struct igf_nxp_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	IGF_Init(config->base);

	for (uint8_t i = 0; i < config->num_channels; i++) {
		const struct igf_nxp_channel_config *chan = &config->channels[i];

		/*
		 * The channel is still disabled at this point, satisfying the
		 * fsl_igf requirement that the filter configuration is changed
		 * only while the channel is disabled.
		 */
		IGF_SetIgfConfig(config->base, chan->channel, &chan->cfg);
		IGF_Enable(config->base, chan->channel, true);
	}

	return 0;
}

/* Validate every configured channel index at build time. */
#define IGF_CHANNEL_ASSERT(child_node)                                                             \
	BUILD_ASSERT(DT_REG_ADDR(child_node) < IGF_CHANNEL_MAX,                                    \
		     "nxp,imx-igf channel index must be 0..31");

/* Build one igf_nxp_channel_config entry from a devicetree channel node. */
#define IGF_CHANNEL_ENTRY(child_node)                                                              \
	{                                                                                          \
		.channel = DT_REG_ADDR(child_node),                                                \
		.cfg = {                                                                           \
			.enableFilter = true,                                                      \
			.invertOutput = DT_PROP(child_node, invert_output),                        \
			.immediatePropagation = DT_PROP(child_node, immediate_propagation),        \
			.useExternalPrescaler = DT_PROP(child_node, external_prescaler),           \
			.prescaler = DT_PROP(child_node, prescaler),                               \
			.risingEdgeFilter = DT_ENUM_IDX(child_node, rising_filter),                \
			.fallingEdgeFilter = DT_ENUM_IDX(child_node, falling_filter),              \
			.risingThreshold = DT_PROP(child_node, rising_threshold),                  \
			.fallingThreshold = DT_PROP(child_node, falling_threshold),                \
		},                                                                                 \
	},

#define IGF_NXP_INIT(n)                                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IGF_CHANNEL_ASSERT)                                   \
	static const struct igf_nxp_channel_config igf_nxp_channels_##n[] = {                      \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, IGF_CHANNEL_ENTRY)                            \
	};                                                                                         \
	static const struct igf_nxp_config igf_nxp_config_##n = {                                  \
		.base = (IGF_Type *)DT_INST_REG_ADDR(n),                                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)COND_CODE_1(                               \
			DT_PHA_HAS_CELL(DT_DRV_INST(n), clocks, name),                             \
			(DT_INST_CLOCKS_CELL(n, name)), (0U)),                                     \
		.channels = igf_nxp_channels_##n,                                                  \
		.num_channels = ARRAY_SIZE(igf_nxp_channels_##n),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, igf_nxp_init, NULL, NULL, &igf_nxp_config_##n, POST_KERNEL,       \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(IGF_NXP_INIT)
