/*
 * Copyright (c) 2022, Valerio Setti <vsetti@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_qdec

/** @file
 * @brief STM32 family Quadrature Decoder (QDEC) driver.
 */

#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/logging/log.h>

#include <stm32_ll_tim.h>

LOG_MODULE_REGISTER(qdec_stm32, CONFIG_SENSOR_LOG_LEVEL);

/* Device constant configuration parameters */
struct qdec_stm32_dev_cfg {
	const struct pinctrl_dev_config *pin_config;
	struct stm32_pclken pclken;
	TIM_TypeDef *timer_inst;
	uint32_t encoder_mode;
	bool is_input_polarity_inverted;
	uint8_t input_filtering_level;
	uint32_t counts_per_revolution;
};

/* Device run time data */
struct qdec_stm32_dev_data {
	uint32_t position;
	uint32_t counts;
};

static int qdec_stm32_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct qdec_stm32_dev_data *dev_data = dev->data;
	const struct qdec_stm32_dev_cfg *dev_cfg = dev->config;
	uint32_t total_counter_value;
	uint32_t counter_value;

	if ((chan != SENSOR_CHAN_ALL) &&
	    (chan != SENSOR_CHAN_ROTATION) && (chan != SENSOR_CHAN_ENCODER_COUNT)) {
		return -ENOTSUP;
	}

	total_counter_value = LL_TIM_GetCounter(dev_cfg->timer_inst);
	dev_data->counts = total_counter_value;

	/* We're only interested in the remainder between the current counter value and
	 * counts_per_revolution. The integer part represents an entire rotation so it
	 * can be ignored
	 */
	counter_value = total_counter_value % dev_cfg->counts_per_revolution;

	/* The angle calculated in the fixed-point format (Q26.6 format) */
	dev_data->position = (counter_value * 23040) / dev_cfg->counts_per_revolution;

	return 0;
}

static int qdec_stm32_get(const struct device *dev, enum sensor_channel chan,
			  struct sensor_value *val)
{
	struct qdec_stm32_dev_data *const dev_data = dev->data;

	if (chan == SENSOR_CHAN_ROTATION) {
		val->val1 = dev_data->position >> 6;
		val->val2 = (dev_data->position & 0x3F) * 15625;
	} else if (chan == SENSOR_CHAN_ENCODER_COUNT) {
		val->val1 = dev_data->counts;
		val->val2 = 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void qdec_stm32_initialize_channel(const struct device *dev, uint32_t ll_channel)
{
	const struct qdec_stm32_dev_cfg *const dev_cfg = dev->config;

	LL_TIM_IC_SetActiveInput(dev_cfg->timer_inst, ll_channel, LL_TIM_ACTIVEINPUT_DIRECTTI);
	LL_TIM_IC_SetFilter(dev_cfg->timer_inst, ll_channel,
			    dev_cfg->input_filtering_level * LL_TIM_IC_FILTER_FDIV1_N2);
	LL_TIM_IC_SetPrescaler(dev_cfg->timer_inst, ll_channel, LL_TIM_ICPSC_DIV1);
	LL_TIM_IC_SetPolarity(dev_cfg->timer_inst, ll_channel,
			      dev_cfg->is_input_polarity_inverted ? LL_TIM_IC_POLARITY_FALLING :
								    LL_TIM_IC_POLARITY_RISING);
}

static int qdec_stm32_initialize(const struct device *dev)
{
	const struct qdec_stm32_dev_cfg *const dev_cfg = dev->config;
	int retval;
	uint32_t max_counter_value;

	retval = pinctrl_apply_state(dev_cfg->pin_config, PINCTRL_STATE_DEFAULT);
	if (retval < 0) {
		return retval;
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	retval = clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				  (clock_control_subsys_t)&dev_cfg->pclken);
	if (retval < 0) {
		LOG_ERR("Could not initialize clock");
		return retval;
	}

	/* Ensure that the counter will always count up to a multiple of counts_per_revolution */
	if (IS_TIM_32B_COUNTER_INSTANCE(dev_cfg->timer_inst)) {
		max_counter_value = UINT32_MAX - (UINT32_MAX % dev_cfg->counts_per_revolution) - 1;
	} else {
		max_counter_value = UINT16_MAX - (UINT16_MAX % dev_cfg->counts_per_revolution) - 1;
	}
	LL_TIM_SetAutoReload(dev_cfg->timer_inst, max_counter_value);

	LL_TIM_SetClockSource(dev_cfg->timer_inst, dev_cfg->encoder_mode);

	qdec_stm32_initialize_channel(dev, LL_TIM_CHANNEL_CH1);
	qdec_stm32_initialize_channel(dev, LL_TIM_CHANNEL_CH2);

	LL_TIM_CC_EnableChannel(dev_cfg->timer_inst, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2);

	LL_TIM_EnableCounter(dev_cfg->timer_inst);

	return 0;
}

static DEVICE_API(sensor, qdec_stm32_driver_api) = {
	.sample_fetch = qdec_stm32_fetch,
	.channel_get = qdec_stm32_get,
};

#define QDEC_STM32_INIT(n)                                                                         \
	BUILD_ASSERT(DT_INST_PROP(n, st_counts_per_revolution) > 0,                                \
		     "Counts per revolution must be above 0");                                     \
                                                                                                   \
	BUILD_ASSERT(!(DT_INST_PROP(n, st_encoder_mode) & ~TIM_SMCR_SMS),                          \
		     "Encoder mode is not supported by this MCU");                                 \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct qdec_stm32_dev_cfg qdec##n##_stm32_config = {                          \
		.pin_config = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                   \
		.timer_inst = ((TIM_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(n))),                     \
		.pclken = STM32_CLOCK_INFO(0, DT_INST_PARENT(n)),				   \
		.encoder_mode = DT_INST_PROP(n, st_encoder_mode),                                  \
		.is_input_polarity_inverted = DT_INST_PROP(n, st_input_polarity_inverted),         \
		.input_filtering_level = DT_INST_PROP(n, st_input_filter_level),                   \
		.counts_per_revolution = DT_INST_PROP(n, st_counts_per_revolution),                \
	};                                                                                         \
                                                                                                   \
	static struct qdec_stm32_dev_data qdec##n##_stm32_data;                                    \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, qdec_stm32_initialize, NULL, &qdec##n##_stm32_data,        \
				     &qdec##n##_stm32_config, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &qdec_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_STM32_INIT)
