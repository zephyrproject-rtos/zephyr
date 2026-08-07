/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <sl_hal_vdac.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(silabs_vdac, CONFIG_DAC_LOG_LEVEL);

#define DT_DRV_COMPAT silabs_vdac

#define NUM_CHANNELS	2
#define MAX_FREQUENCY	1000000

/* Read-only driver configuration */
struct vdac_config {
	VDAC_TypeDef *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	sl_hal_vdac_init_t init;
	sl_hal_vdac_init_channel_t channel_init[NUM_CHANNELS];
};

static int vdac_init(const struct device *dev)
{
	const struct vdac_config *config = dev->config;
	sl_hal_vdac_init_t init = config->init;
	uint32_t freq;
	int err;

	/* Configure pinctrl */
	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0 && err != -ENOENT) {
		LOG_ERR("failed to allocate silabs,analog-bus via pinctrl");
		return err;
	}

	/* Enable VDAC Clock */
	err = clock_control_on(config->clock_dev,
			       (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		LOG_ERR("failed to enable clocks via clock_control");
		return err;
	}

	/* Calculate clock prescaler */
	err = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)&config->clock_cfg, &freq);
	if (err < 0) {
		LOG_ERR("failed to get clock rate via clock_control");
		return err;
	}
	init.prescaler = sl_hal_vdac_calculate_prescaler(config->base, MAX_FREQUENCY, freq);

	/* Initialize VDAC */
	sl_hal_vdac_init(config->base, &init);

	return 0;
}

static int vdac_channel_setup(const struct device *dev, const struct dac_channel_cfg *channel_cfg)
{
	const struct vdac_config *config = dev->config;

	if (channel_cfg->channel_id >= NUM_CHANNELS) {
		LOG_ERR("unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != VDAC_RESOLUTION(VDAC_NUM(config->base))) {
		LOG_ERR("unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("internal channels not supported");
		return -ENOTSUP;
	}

	/* Configure channel */
	sl_hal_vdac_init_channel(config->base,
				 &config->channel_init[channel_cfg->channel_id],
				 channel_cfg->channel_id);

	/* Start channel */
	sl_hal_vdac_enable_channel(config->base, channel_cfg->channel_id);

	return 0;
}

static int vdac_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct vdac_config *config = dev->config;

	if (channel >= NUM_CHANNELS) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (value >= (1 << VDAC_RESOLUTION(VDAC_NUM(config->base)))) {
		LOG_ERR("value %d out of range", value);
		return -ENOTSUP;
	}

	/* Write value to VDAC channel */
	sl_hal_vdac_set_output_channel(config->base, channel, value);

	return 0;
}

static DEVICE_API(dac, vdac_api) = {
	.channel_setup = vdac_channel_setup,
	.write_value = vdac_write_value,
};

#define VDAC_CHANNEL(node)							\
	.channel_init[DT_REG_ADDR(node)] = {					\
		.main_out_enable = DT_PROP(node, main_output),			\
		.aux_out_enable = DT_NODE_HAS_PROP(node, aux_output),		\
		.short_output = DT_PROP(node, short_output),			\
		.power_mode = DT_PROP(node, low_power_mode),			\
		.high_cap_load_enable = DT_PROP(node, high_capacitance_load),	\
		.port =	DT_PROP_OR(node, aux_output, 0) >> 4,			\
		.pin = DT_PROP_OR(node, aux_output, 0) & 0xF,			\
		.sample_off_mode = DT_PROP(node, sample_off_mode),		\
		.hold_out_time = DT_PROP(node, output_hold_cycles),		\
		.ch_refresh_source = DT_PROP(node, refresh_timer),		\
		.trigger_mode = SL_HAL_VDAC_TRIGGER_MODE_SW,			\
	},

#define VDAC_DEVICE(inst)							\
										\
	PINCTRL_DT_INST_DEFINE(inst);						\
										\
	static const struct vdac_config vdac_config_##inst = {			\
		.base = (VDAC_TypeDef *)DT_INST_REG_ADDR(inst),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),		\
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(inst),			\
		.init = SL_HAL_VDAC_INIT_DEFAULT,				\
		.init.reference = DT_INST_ENUM_IDX(inst, voltage_reference),	\
		.init.warmup_time = DT_INST_PROP(inst, warmup_cycles),		\
		.init.refresh = DT_INST_ENUM_IDX(inst, refresh_period_cycles),	\
		DT_INST_FOREACH_CHILD(inst, VDAC_CHANNEL)			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, &vdac_init, PM_DEVICE_DT_INST_GET(inst),	\
			      NULL, &vdac_config_##inst, POST_KERNEL,		\
			      CONFIG_DAC_INIT_PRIORITY, &vdac_api);

DT_INST_FOREACH_STATUS_OKAY(VDAC_DEVICE)
