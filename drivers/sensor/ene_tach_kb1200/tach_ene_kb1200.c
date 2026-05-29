/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_tach

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <reg/tacho.h>

/* Device config */
struct tach_kb1200_config {
	/* tachometer controller base address */
	struct tacho_regs *tacho;
	/* number of pulses (holes) per	round of tachometer's input (encoder) */
	int pulses_per_round;
	/* sampling clock timing of tachometer (us) */
	int sample_time_us;
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct tach_kb1200_data {
	/* Captured counts of tachometer */
	uint32_t capture;
};

/* TACH	local functions	*/
static int tach_kb1200_configure(const struct device *dev)
{
	const struct tach_kb1200_config *const config = dev->config;
	uint8_t sample_us = 0;

	/* Configure clock module and its frequency of tachometer */
	switch (config->sample_time_us) {
	case 2:
		sample_us = TACHO_MONITOR_CLK_2US;
		break;
	case 8:
		sample_us = TACHO_MONITOR_CLK_8US;
		break;
	case 16:
		sample_us = TACHO_MONITOR_CLK_16US;
		break;
	case 64:
		sample_us = TACHO_MONITOR_CLK_64US;
		break;
	default:
		return -ENOTSUP;
	}
	config->tacho->TACHOCFG = (sample_us << 4) | TACHO_FUNCTION_ENABLE;

	return 0;
}

/* TACH	api functions */
int tach_kb1200_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);
	struct tach_kb1200_data *const data = dev->data;
	const struct tach_kb1200_config *const config = dev->config;

	/* Check tachometer timeout flag*/
	if (config->tacho->TACHOPF & TACHO_TIMEOUT_EVENT) {
		/* Clear timeout flags and update flag */
		config->tacho->TACHOPF = (TACHO_TIMEOUT_EVENT | TACHO_UPDATE_EVENT);
		data->capture = 0;
		return 0;
	}

	/* Check tachometer update flag	is set */
	if (config->tacho->TACHOPF & TACHO_UPDATE_EVENT) {
		/* Clear pending flags */
		config->tacho->TACHOPF = TACHO_UPDATE_EVENT;
		/* Save	captured count */
		data->capture = config->tacho->TACHOCV & TACHO_CNT_MAX_VALUE;
	}
	return 0;
}

static int tach_kb1200_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct tach_kb1200_data *const data = dev->data;
	const struct tach_kb1200_config *const config = dev->config;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	if (data->capture > 0) {
		/*
		 *   RPM = (60000000/t) / n
		 *   t:	One Pulses length(us) =	sample_time_us * cnt
		 *   n:	One Round pulses Number
		 */
		val->val1 = (60000000 / (config->sample_time_us * data->capture)) /
			    config->pulses_per_round;
	} else {
		val->val1 = 0U;
	}
	val->val2 = 0U;

	return 0;
}

/* TACH	driver registration */
static int tach_kb1200_init(const struct device *dev)
{
	int ret;
	const struct tach_kb1200_config *config = dev->config;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	tach_kb1200_configure(dev);

	return 0;
}

static DEVICE_API(sensor, tach_kb1200_driver_api) = {
	.sample_fetch = tach_kb1200_sample_fetch,
	.channel_get = tach_kb1200_channel_get,
};

#define KB1200_TACH_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct tach_kb1200_config tach_cfg_##inst = {                                 \
		.tacho = (struct tacho_regs *)DT_INST_REG_ADDR(inst),                              \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),                          \
		.sample_time_us = DT_INST_PROP(inst, sample_time_us),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	static struct tach_kb1200_data tach_data_##inst;                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tach_kb1200_init, NULL, &tach_data_##inst,              \
				     &tach_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
				     &tach_kb1200_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_TACH_INIT)
