/*
 * Copyright (c) 2025 Polytech A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tmpsns

#include <fsl_tempsensor.h>
#include <zephyr/drivers/sensor.h>

struct nxp_tmpsns_config {
	TMPSNS_Type *base;
	uint16_t clock_divider;
};

struct nxp_tmpsns_data {
	float die_temp;
};

static int nxp_tmpsns_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	const struct nxp_tmpsns_data *data = dev->data;

	if (chan == SENSOR_CHAN_DIE_TEMP) {
		return sensor_value_from_float(val, data->die_temp);
	}

	return -ENOTSUP;
}

static int nxp_tmpsns_channel_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct nxp_tmpsns_config *cfg = dev->config;
	struct nxp_tmpsns_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_DIE_TEMP:
		data->die_temp = TMPSNS_GetCurrentTemperature(cfg->base);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, tmpsns_driver_api) = {
	.channel_get = nxp_tmpsns_channel_get,
	.sample_fetch = nxp_tmpsns_channel_fetch,
};

static int nxp_tmpsns_init(const struct device *dev)
{
	tmpsns_config_t config;

	const struct nxp_tmpsns_config *cfg = dev->config;

	TMPSNS_GetDefaultConfig(&config);
	config.measureMode = kTEMPSENSOR_ContinuousMode;
	config.frequency = cfg->clock_divider;
	TMPSNS_Init(cfg->base, &config);
	TMPSNS_StartMeasure(cfg->base);

	return 0;
}

#define NXP_TMPSNS_DEFINE(inst)                                                                    \
	static struct nxp_tmpsns_config nxp_tmpsns_config_##inst = {                               \
		.base = (TMPSNS_Type *)DT_INST_REG_ADDR(inst),                                     \
		.clock_divider = DT_INST_PROP(0, clock_divider)};                                  \
	struct nxp_tmpsns_data nxp_tmpsns_data_##inst;                                             \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, nxp_tmpsns_init, NULL, &nxp_tmpsns_data_##inst,         \
				     &nxp_tmpsns_config_##inst, POST_KERNEL,                       \
				     CONFIG_SENSOR_INIT_PRIORITY, &tmpsns_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_TMPSNS_DEFINE)
