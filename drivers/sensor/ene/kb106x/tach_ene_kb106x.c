/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb106x_tach

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <reg/tacho.h>

/* Device config */
struct tach_kb106x_config {
	/* tachometer controller base address */
	struct tacho_regs *tacho;
	/* number of pulses (holes) per	round of tachometer's input (encoder) */
	int pulses_per_round;
	/* sampling clock timing of tachometer (us) */
	int sample_time_us;
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct tach_kb106x_data {
	/* Captured counts of tachometer */
	uint32_t capture;
};

/* TACH	local functions	*/
static int tach_kb106x_configure(const struct device *dev)
{
	const struct tach_kb106x_config *const config = dev->config;
	uint8_t sample_us = 0;

	/* Configure clock module and its frequency of tachometer */
	switch (config->sample_time_us) {
	case 8:
		sample_us = TACHO_MONITOR_CLK_8US;
		break;
	case 16:
		sample_us = TACHO_MONITOR_CLK_16US;
		break;
	case 32:
		sample_us = TACHO_MONITOR_CLK_32US;
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
int tach_kb106x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);
	struct tach_kb106x_data *const data = dev->data;
	const struct tach_kb106x_config *const config = dev->config;
	uint32_t pending_flags;

	pending_flags = config->tacho->TACHOPF;
	/* Check tachometer timeout flag*/
	if (pending_flags & TACHO_TIMEOUT_EVENT) {
		/* patch for timeout flags and update flag set up together */
		if (!(pending_flags & TACHO_UPDATE_EVENT)) {
			data->capture = 0;
		}
	}

	/* Check tachometer update flag	is set */
	if (pending_flags & TACHO_UPDATE_EVENT) {
		/* Save	captured count */
		data->capture = config->tacho->TACHOCV & TACHO_CNT_MAX_VALUE;
	}

	/* Clear pending flags */
	config->tacho->TACHOPF = pending_flags;
	return 0;
}

static int tach_kb106x_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct tach_kb106x_data *const data = dev->data;
	const struct tach_kb106x_config *const config = dev->config;

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
static int tach_kb106x_init(const struct device *dev)
{
	int ret;
	const struct tach_kb106x_config *config = dev->config;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = tach_kb106x_configure(dev);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, tach_kb106x_driver_api) = {
	.sample_fetch = tach_kb106x_sample_fetch,
	.channel_get = tach_kb106x_channel_get,
};

#define KB106X_TACH_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct tach_kb106x_config tach_cfg_##inst = {                                 \
		.tacho = (struct tacho_regs *)DT_INST_REG_ADDR(inst),                              \
		.pulses_per_round = DT_INST_PROP(inst, pulses_per_round),                          \
		.sample_time_us = DT_INST_PROP(inst, sample_time_us),                              \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	static struct tach_kb106x_data tach_data_##inst;                                           \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tach_kb106x_init, NULL, &tach_data_##inst,              \
				     &tach_cfg_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
				     &tach_kb106x_driver_api);
DT_INST_FOREACH_STATUS_OKAY(KB106X_TACH_INIT)
