/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_temp

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <ti/driverlib/m0p/dl_sysctl.h>

LOG_MODULE_REGISTER(mspm0_temp, CONFIG_SENSOR_LOG_LEVEL);

/* TEMP_SENSE0 is a 12-bit ADC code */
#define MSPM0_TEMP_TRIM_RESOLUTION 12U

struct mspm0_temp_config {
	struct adc_dt_spec adc;
	int32_t trim_temp;
	int32_t trim_vref_mv;
	int32_t coefficient_uv;
};

struct mspm0_temp_data {
	int32_t trim_uv;
	/* Last measured die temperature in milli-degrees Celsius */
	int32_t temp_mc;
};

static int mspm0_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct mspm0_temp_config *cfg = dev->config;
	struct mspm0_temp_data *data = dev->data;
	uint16_t sample;
	struct adc_sequence seq = {
		.buffer = &sample,
		.buffer_size = sizeof(sample),
	};
	int32_t uv;
	int ret;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_DIE_TEMP)) {
		return -ENOTSUP;
	}

	ret = adc_sequence_init_dt(&cfg->adc, &seq);
	if (ret < 0) {
		return ret;
	}

	ret = adc_read_dt(&cfg->adc, &seq);
	if (ret < 0) {
		return ret;
	}

	uv = sample;
	ret = adc_raw_to_microvolts_dt(&cfg->adc, &uv);
	if (ret < 0) {
		return ret;
	}

	/* T = TSTRIM + (V_sample - V_trim) / TSc */
	data->temp_mc = (cfg->trim_temp * 1000) +
			(int32_t)((((int64_t)uv - data->trim_uv) * 1000) / cfg->coefficient_uv);

	return 0;
}

static int mspm0_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct mspm0_temp_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = data->temp_mc / 1000;
	val->val2 = (data->temp_mc % 1000) * 1000;

	return 0;
}

static DEVICE_API(sensor, mspm0_temp_api) = {
	.sample_fetch = mspm0_temp_sample_fetch,
	.channel_get = mspm0_temp_channel_get,
};

static int mspm0_temp_init(const struct device *dev)
{
	const struct mspm0_temp_config *cfg = dev->config;
	struct mspm0_temp_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&cfg->adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	if (!cfg->adc.channel_cfg_dt_node_exists ||
	    (cfg->adc.channel_cfg.reference != ADC_REF_INTERNAL) ||
	    (adc_ref_internal(cfg->adc.dev) == 0U)) {
		LOG_ERR("ADC channel %u must use the internal reference", cfg->adc.channel_id);
		return -EINVAL;
	}

	ret = adc_channel_setup_dt(&cfg->adc);
	if (ret < 0) {
		LOG_ERR("ADC channel setup failed: %d", ret);
		return ret;
	}

	data->trim_uv = (int32_t)(((int64_t)DL_SYSCTL_getTempCalibrationConstant() *
				   cfg->trim_vref_mv * 1000) >>
				  MSPM0_TEMP_TRIM_RESOLUTION);

	return 0;
}

#define MSPM0_TEMP_DEFINE(inst)                                                                    \
	static const struct mspm0_temp_config mspm0_temp_config_##inst = {                         \
		.adc = ADC_DT_SPEC_INST_GET(inst),                                                 \
		.trim_temp = DT_INST_PROP(inst, ts_trim_temp),                                     \
		.trim_vref_mv = DT_INST_PROP(inst, ts_trim_vref),                                  \
		.coefficient_uv = DT_INST_PROP(inst, ts_coefficient),                              \
	};                                                                                         \
	static struct mspm0_temp_data mspm0_temp_data_##inst;                                      \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mspm0_temp_init, NULL, &mspm0_temp_data_##inst,         \
				     &mspm0_temp_config_##inst, POST_KERNEL,                       \
				     CONFIG_SENSOR_INIT_PRIORITY, &mspm0_temp_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_TEMP_DEFINE)
