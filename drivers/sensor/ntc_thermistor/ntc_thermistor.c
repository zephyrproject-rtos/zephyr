/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "ntc_thermistor.h"

LOG_MODULE_REGISTER(NTC_THERMISTOR, CONFIG_SENSOR_LOG_LEVEL);

struct ntc_thermistor_data {
	struct k_mutex mutex;
	int16_t raw;
	int16_t sample_val;
};

struct ntc_thermistor_config {
	const struct adc_dt_spec adc_channel;
	const struct ntc_config ntc_cfg;
};

static int ntc_thermistor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ntc_thermistor_data *data = dev->data;
	const struct ntc_thermistor_config *cfg = dev->config;
	int32_t val_mv;
	int res;
	struct adc_sequence sequence = {
		.options = NULL,
		.buffer = &data->raw,
		.buffer_size = sizeof(data->raw),
		.calibrate = false,
	};

	k_mutex_lock(&data->mutex, K_FOREVER);

	adc_sequence_init_dt(&cfg->adc_channel, &sequence);
	res = adc_read(cfg->adc_channel.dev, &sequence);
	if (res) {
		val_mv = data->raw;
		res = adc_raw_to_millivolts_dt(&cfg->adc_channel, &val_mv);
		data->sample_val = val_mv;
	}

	k_mutex_unlock(&data->mutex);

	return res;
}

static int ntc_thermistor_channel_get(const struct device *dev, enum sensor_channel chan,
				      struct sensor_value *val)
{
	struct ntc_thermistor_data *data = dev->data;
	const struct ntc_thermistor_config *cfg = dev->config;
	uint32_t ohm, max_adc;
	int32_t temp;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		max_adc = (1 << (cfg->adc_channel.resolution - 1)) - 1;
		ohm = ntc_get_ohm_of_thermistor(&cfg->ntc_cfg, max_adc, data->raw);
		temp = ntc_get_temp_mc(&cfg->ntc_cfg.type, ohm);
		val->val1 = temp / 1000;
		val->val2 = (temp % 1000) * 1000;
		LOG_INF("ntc temp says %u", val->val1);
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static const struct sensor_driver_api ntc_thermistor_driver_api = {
	.sample_fetch = ntc_thermistor_sample_fetch,
	.channel_get = ntc_thermistor_channel_get,
};

static int ntc_thermistor_init(const struct device *dev)
{
	const struct ntc_thermistor_config *cfg = dev->config;
	int err;

	if (!device_is_ready(cfg->adc_channel.dev)) {
		LOG_ERR("ADC controller device is not ready\n");
		return -ENODEV;
	}

	err = adc_channel_setup_dt(&cfg->adc_channel);
	if (err < 0) {
		LOG_ERR("Could not setup channel err(%d)\n", err);
		return err;
	}

	return 0;
}

#define NTC_THERMISTOR_DEFINE(inst, id, comp_table)                                                \
	BUILD_ASSERT(ARRAY_SIZE(comp_table) % 2 == 0, "Compensation table needs to be even size"); \
                                                                                                   \
	static int compare_ohm_##id##inst(const void *key, const void *element);                   \
                                                                                                   \
	static struct ntc_thermistor_data ntc_thermistor_driver_##id##inst;                        \
                                                                                                   \
	static const struct ntc_thermistor_config ntc_thermistor_cfg_##id##inst = {                \
		.adc_channel = ADC_DT_SPEC_INST_GET(inst),                                         \
		.ntc_cfg =                                                                         \
			{                                                                          \
				.r25_ohm = DT_INST_PROP(inst, r25_ohm),                            \
				.pullup_uv = DT_INST_PROP(inst, pullup_uv),                        \
				.pullup_ohm = DT_INST_PROP(inst, pullup_ohm),                      \
				.pulldown_ohm = DT_INST_PROP(inst, pulldown_ohm),                  \
				.connected_positive = DT_INST_PROP(inst, connected_positive),      \
				.type = {                                                          \
					.comp = (const struct ntc_compensation *)comp_table,       \
					.n_comp = ARRAY_SIZE(comp_table),                          \
					.ohm_cmp = compare_ohm_##id##inst,                         \
				},                                                                 \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static int compare_ohm_##id##inst(const void *key, const void *element)                    \
	{                                                                                          \
		return ntc_compensation_compare_ohm(                                               \
			&ntc_thermistor_cfg_##id##inst.ntc_cfg.type, key, element);                \
	}                                                                                          \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		inst, ntc_thermistor_init, NULL, &ntc_thermistor_driver_##id##inst,                \
		&ntc_thermistor_cfg_##id##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,          \
		&ntc_thermistor_driver_api);

/* ntc-thermistor-generic */
#define DT_DRV_COMPAT ntc_thermistor_generic

#define NTC_THERMISTOR_GENERIC_DEFINE(inst)                                                        \
	static const uint32_t comp_##inst[] = DT_INST_PROP(inst, zephyr_compensation_table);       \
	NTC_THERMISTOR_DEFINE(inst, DT_DRV_COMPAT, comp_##inst)

DT_INST_FOREACH_STATUS_OKAY(NTC_THERMISTOR_GENERIC_DEFINE)

/* epcos,b57861s0103a039 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT epcos_b57861s0103a039

static __unused const struct ntc_compensation comp_epcos_b57861s0103a039[] = {
	{ -25, 146676 },
	{ -15, 78875 },
	{ -5, 44424 },
	{ 5, 26075 },
	{ 15, 15881 },
	{ 25, 10000 },
	{ 35, 6488 },
	{ 45, 4326 },
	{ 55, 2956 },
	{ 65, 2066 },
	{ 75, 1474 },
	{ 85, 1072 },
	{ 95, 793 },
	{ 105, 596 },
	{ 115, 454 },
	{ 125, 351 },
};

DT_INST_FOREACH_STATUS_OKAY_VARGS(NTC_THERMISTOR_DEFINE, DT_DRV_COMPAT,
				  comp_epcos_b57861s0103a039)
