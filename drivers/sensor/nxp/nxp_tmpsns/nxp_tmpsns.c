/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "math.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>

#define DT_DRV_COMPAT nxp_tmpsns

#define NXP_TMPSNS_PARAMS_TS20		133.6f
#define NXP_TMPSNS_PARAMS_TS21		-5.39f
#define NXP_TMPSNS_PARAMS_TS21_SQUARE	29.0521f
#define NXP_TMPSNS_PARAMS_TS22		0.002f

struct nxp_tmpsns_data {
	/* Temperature value at 25C, the value for
	 * this bit field comes from the fuses.
	 */
	uint16_t tmp_25c;

	/* Measured code output from the temperature sensor. */
	uint32_t tmp_measure;
};

struct nxp_tmpsns_config {
	TMPSNS_Type *base;
};

static int nxp_tmpsns_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct nxp_tmpsns_config *cfg = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		cfg->base->CTRL1 |= TMPSNS_CTRL1_SET_START_MASK;
		return 0;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		cfg->base->CTRL1 &= ~TMPSNS_CTRL1_SET_START_MASK;
		return 0;
	}

	return -ENOTSUP;
}

static int nxp_tmpsns_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct nxp_tmpsns_data *data = dev->data;
	const struct nxp_tmpsns_config *cfg = dev->config;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	while (TMPSNS_STATUS0_FINISH_MASK != (cfg->base->STATUS0 & TMPSNS_STATUS0_FINISH_MASK)) {
	}

	data->tmp_measure = (cfg->base->STATUS0 & TMPSNS_STATUS0_TEMP_VAL_MASK) >>
			TMPSNS_STATUS0_TEMP_VAL_SHIFT;

	cfg->base->STATUS0 = TMPSNS_STATUS0_FINISH_MASK;

	return 0;
}

static int nxp_tmpsns_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct nxp_tmpsns_data *data = dev->data;
	float tmp;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	tmp = (-NXP_TMPSNS_PARAMS_TS21 - sqrtf(NXP_TMPSNS_PARAMS_TS21_SQUARE -
		4.0f * NXP_TMPSNS_PARAMS_TS22 * (NXP_TMPSNS_PARAMS_TS20 + data->tmp_25c
		- (float)(data->tmp_measure)))) / (2.0f * NXP_TMPSNS_PARAMS_TS22);

	return sensor_value_from_double(val, tmp);
}

static int nxp_tmpsns_init(const struct device *dev)
{
	struct nxp_tmpsns_data *data = dev->data;
	const struct nxp_tmpsns_config *cfg = dev->config;

	cfg->base->CTRL1 &= (~(TMPSNS_CTRL1_FREQ_MASK | TMPSNS_CTRL1_PWD_MASK |
			TMPSNS_CTRL1_PWD_FULL_MASK));

	data->tmp_25c = (ANADIG_TEMPSENSOR->TEMPSNS_OTP_TRIM_VALUE &
		ANADIG_TEMPSENSOR_TEMPSNS_OTP_TRIM_VALUE_TEMPSNS_TEMP_VAL_MASK) >>
		ANADIG_TEMPSENSOR_TEMPSNS_OTP_TRIM_VALUE_TEMPSNS_TEMP_VAL_SHIFT;

	return pm_device_driver_init(dev, nxp_tmpsns_pm_callback);
}

static DEVICE_API(sensor, nxp_tmpsns_api) = {
	.sample_fetch = nxp_tmpsns_sample_fetch,
	.channel_get = nxp_tmpsns_channel_get,
};

#define NXP_TMPSNS_INIT(inst)							\
	static struct nxp_tmpsns_data _CONCAT(data, inst) = {			\
		.tmp_25c = 0,							\
		.tmp_measure = 0,						\
	};									\
										\
	static const struct nxp_tmpsns_config _CONCAT(config, inst) = {		\
		.base = (TMPSNS_Type *)DT_INST_REG_ADDR(inst),			\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(inst, nxp_tmpsns_pm_callback);			\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, nxp_tmpsns_init,			\
				PM_DEVICE_DT_INST_GET(inst),			\
				&_CONCAT(data, inst), &_CONCAT(config, inst),	\
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,	\
				&nxp_tmpsns_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_TMPSNS_INIT)
