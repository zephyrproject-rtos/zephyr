/*
 * Copyright (c) 2025 Pivot International Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/qdec_renesas_ra.h>
#include <zephyr/drivers/pinctrl.h>
#include "r_gpt.h"
#include "r_gpt_cfg.h"
#include <zephyr/logging/log.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LOG_MODULE_REGISTER(qdec_renesas_ra, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_ra_qdec

struct qdec_renesas_ra_data {
	gpt_instance_ctrl_t fsp_ctrl;
	timer_cfg_t fsp_cfg;
	gpt_extended_cfg_t extend_cfg;
	double micro_ticks_per_rev;

	int32_t counts;
};

struct qdec_renesas_ra_config {
	const struct device *clock_dev;
	struct clock_control_ra_subsys_cfg clock_subsys;
	const struct pinctrl_dev_config *pincfg;
};

static int qdec_renesas_ra_attr_set(const struct device *dev, enum sensor_channel chan,
				     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct qdec_renesas_ra_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) &&
	    (chan != SENSOR_CHAN_ROTATION) && (chan != SENSOR_CHAN_ENCODER_COUNT)) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_renesas_ra)attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		data->micro_ticks_per_rev = val->val1 / 1000000.0f;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdec_renesas_ra_attr_get(const struct device *dev, enum sensor_channel chan,
				     enum sensor_attribute attr, struct sensor_value *val)
{
	struct qdec_renesas_ra_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) &&
	    (chan != SENSOR_CHAN_ROTATION) && (chan != SENSOR_CHAN_ENCODER_COUNT)) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_renesas_ra)attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		val->val1 = data->micro_ticks_per_rev * 1000000;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdec_renesas_ra_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct qdec_renesas_ra_data *const data = dev->data;
	timer_status_t stat;

	if ((chan != SENSOR_CHAN_ALL) &&
	    (chan != SENSOR_CHAN_ROTATION) && (chan != SENSOR_CHAN_ENCODER_COUNT)) {
		return -ENOTSUP;
	}

	/* Read position register content */
	R_GPT_StatusGet(&data->fsp_ctrl, &stat);
	data->counts = (int32_t)stat.counter;

	return 0;
}

static int qdec_renesas_ra_get(const struct device *dev, enum sensor_channel chan,
			struct sensor_value *val)
{
	struct qdec_renesas_ra_data *const data = dev->data;
	double rotation = (data->counts * 2.0 * M_PI) / (data->micro_ticks_per_rev);

	switch (chan) {
	case SENSOR_CHAN_ROTATION:
		sensor_value_from_double(val, rotation);
		break;
	case SENSOR_CHAN_ENCODER_COUNT:
		val->val1 = data->counts;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int qdec_renesas_ra_init(const struct device *dev)
{
	struct qdec_renesas_ra_data *data = dev->data;
	const struct qdec_renesas_ra_config *cfg = dev->config;
	int err;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = clock_control_on(cfg->clock_dev, (clock_control_subsys_t)&cfg->clock_subsys);
	if (err < 0) {
		LOG_ERR("Could not initialize clock (%d)", err);
		return err;
	}

	err = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure pins for PWM (%d)", err);
		return err;
	}

	data->fsp_cfg.p_context = (void *)dev;
	data->fsp_cfg.p_extend = &data->extend_cfg;

	err = R_GPT_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Enable capture source */
	err = R_GPT_Enable(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	/* Start timer */
	err = R_GPT_Start(&data->fsp_ctrl);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(sensor, qdec_renesas_ra_driver_api) = {
	.attr_set = &qdec_renesas_ra_attr_set,
	.attr_get = &qdec_renesas_ra_attr_get,
	.sample_fetch = qdec_renesas_ra_fetch,
	.channel_get = qdec_renesas_ra_get,
};

#define QDEC_RENESAS_RA_INIT(index)                                                               \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(index));                                                  \
	static const gpt_extended_cfg_t g_timer1_extend_##index = {                                \
		.gtioca =                                                                          \
			{                                                                          \
				.output_enabled = false,                                           \
				.stop_level = GPT_PIN_LEVEL_LOW,                                   \
			},                                                                         \
		.gtiocb =                                                                          \
			{                                                                          \
				.output_enabled = false,                                           \
				.stop_level = GPT_PIN_LEVEL_LOW,                                   \
			},                                                                         \
		.start_source = (gpt_source_t)(GPT_SOURCE_NONE),                                   \
		.stop_source = (gpt_source_t)(GPT_SOURCE_NONE),                                    \
		.clear_source = (gpt_source_t)(GPT_SOURCE_NONE),                                   \
		.count_up_source = (gpt_source_t)                                                  \
		(                                                                                  \
			GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_LOW |                                \
			GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_HIGH |                              \
			GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_HIGH |                               \
			GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_LOW                                 \
		),                                                                                 \
		.count_down_source = (gpt_source_t)                                                \
		(                                                                                  \
			GPT_SOURCE_GTIOCB_RISING_WHILE_GTIOCA_LOW |                                \
			GPT_SOURCE_GTIOCB_FALLING_WHILE_GTIOCA_HIGH |                              \
			GPT_SOURCE_GTIOCA_RISING_WHILE_GTIOCB_HIGH |                               \
			GPT_SOURCE_GTIOCA_FALLING_WHILE_GTIOCB_LOW                                 \
		),                                                                                 \
		.capture_a_source = (gpt_source_t)(GPT_SOURCE_NONE),                               \
		.capture_b_source = (gpt_source_t)(GPT_SOURCE_NONE),                               \
		.capture_a_ipl = BSP_IRQ_DISABLED,                                                 \
		.capture_b_ipl = BSP_IRQ_DISABLED,                                                 \
		.capture_a_irq = FSP_INVALID_VECTOR,                                               \
		.capture_b_irq = FSP_INVALID_VECTOR,                                               \
		.capture_filter_gtioca = GPT_CAPTURE_FILTER_NONE,                                  \
		.capture_filter_gtiocb = GPT_CAPTURE_FILTER_NONE,                                  \
		.p_pwm_cfg = NULL,                                                                 \
		.gtior_setting.gtior = (0x0U),                                                     \
		.gtioca_polarity = GPT_GTIOC_POLARITY_NORMAL,                                      \
		.gtiocb_polarity = GPT_GTIOC_POLARITY_NORMAL,                                      \
	};                                                                                         \
	static struct qdec_renesas_ra_data qdec_renesas_ra_data_##index = {                        \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.mode = TIMER_MODE_PERIODIC,                                       \
				.source_div = DT_PROP(DT_INST_PARENT(index), divider),             \
				.channel = DT_PROP(DT_INST_PARENT(index), channel),                \
				.cycle_end_ipl = BSP_IRQ_DISABLED,                                 \
				.cycle_end_irq = FSP_INVALID_VECTOR,                               \
			},                                                                         \
		.extend_cfg = g_timer1_extend_##index,                                             \
		.micro_ticks_per_rev =                                                             \
			(double)(DT_INST_PROP(index, micro_ticks_per_rev) / 1000000.0f)            \
	};                                                                                         \
	static const struct qdec_renesas_ra_config qdec_renesas_ra_config_##index = {              \
		.pincfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(index)),                        \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(index))),                 \
		.clock_subsys = {                                                                  \
			.mstp = (uint32_t)DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(index), 0, mstp),   \
			.stop_bit = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(index), 0, stop_bit),     \
		},                                                                                 \
	};                                                                                         \
	static int qdec_renesas_ra_init_##index(const struct device *dev)                          \
	{                                                                                          \
		int err = qdec_renesas_ra_init(dev);                                               \
		if (err != 0) {                                                                    \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(index, qdec_renesas_ra_init_##index, NULL,                           \
			      &qdec_renesas_ra_data_##index, &qdec_renesas_ra_config_##index,      \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                            \
			      &qdec_renesas_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_RENESAS_RA_INIT);
