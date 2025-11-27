/*
 * Copyright (c) 2025, Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_vbat

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <register.h>

#define SYS_CFG_ANAU_CR offsetof(HPSYS_CFG_TypeDef, ANAU_CR)

LOG_MODULE_REGISTER(sf32lb_vbat, CONFIG_SENSOR_LOG_LEVEL);

struct sf32lb_vbat_config {
	uintptr_t cfg_base;
	const struct device *adc;
	const struct adc_channel_cfg adc_cfg;
	uint32_t ratio;
};

struct sf32lb_vbat_data {
	struct adc_sequence adc_seq;
	struct k_mutex lock;
	int16_t sample_buffer;
	int16_t raw;
};

static int sf32lb_vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct sf32lb_vbat_config *cfg = dev->config;
	struct sf32lb_vbat_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	rc = adc_channel_setup(cfg->adc, &cfg->adc_cfg);
	if (rc < 0) {
		LOG_ERR("ADC channel setup failed (%d)", rc);
		goto out;
	}

	rc = adc_read(cfg->adc, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

out:
	k_mutex_unlock(&data->lock);
	return rc;
}

static int sf32lb_vbat_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	const struct sf32lb_vbat_config *cfg = dev->config;
	struct sf32lb_vbat_data *data = dev->data;
	int32_t voltage;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	voltage = data->raw * adc_ref_internal(cfg->adc) * cfg->ratio / 0x0FFF;

	sensor_value_from_milli(val, voltage);

	return 0;
}

static const struct sensor_driver_api sf32lb_vbat_driver_api = {
	.sample_fetch = sf32lb_vbat_sample_fetch,
	.channel_get = sf32lb_vbat_channel_get,
};

static int sf32lb_vbat_init(const struct device *dev)
{
	const struct sf32lb_vbat_config *cfg = dev->config;
	struct sf32lb_vbat_data *data = dev->data;
	struct adc_sequence *asp = &data->adc_seq;

	sys_set_bit(cfg->cfg_base + SYS_CFG_ANAU_CR, HPSYS_CFG_ANAU_CR_EN_VBAT_MON_Pos);

	k_mutex_init(&data->lock);

	if (!device_is_ready(cfg->adc)) {
		LOG_ERR("ADC device %s is not ready", cfg->adc->name);
		return -ENODEV;
	}

	*asp = (struct adc_sequence){
		.channels = BIT(cfg->adc_cfg.channel_id),
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
		.resolution = 12U,
	};

	return 0;
}

#define SF32LB_VBAT_DEFINE(inst)                                                                   \
	static struct sf32lb_vbat_data sf32lb_vbat_data_##inst;                                    \
                                                                                                   \
	static const struct sf32lb_vbat_config sf32lb_vbat_config_##inst = {                       \
		.cfg_base = DT_REG_ADDR(DT_INST_PHANDLE(inst, sifli_cfg)),                         \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
		.adc_cfg = {                                                                       \
			.gain = ADC_GAIN_1,                                                        \
			.reference = ADC_REF_INTERNAL,                                             \
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,                                  \
			.channel_id = DT_INST_IO_CHANNELS_INPUT(inst),                             \
			.differential = 0,                                                         \
		},                                                                                 \
		.ratio = DT_INST_PROP(inst, ratio),                                                \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sf32lb_vbat_init, NULL, &sf32lb_vbat_data_##inst,       \
				     &sf32lb_vbat_config_##inst, POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY, &sf32lb_vbat_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SF32LB_VBAT_DEFINE)
