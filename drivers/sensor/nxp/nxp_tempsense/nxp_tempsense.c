/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>

#define DT_DRV_COMPAT nxp_tempsense

LOG_MODULE_REGISTER(nxp_tempsense, CONFIG_SENSOR_LOG_LEVEL);

struct nxp_tempsense_config {
	bool expose_ground;
	uint16_t adc_ref_mv;
	TEMPSENSE_Type *base;
	const struct device *adc;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct adc_sequence adc_seq;
	struct adc_channel_cfg ch_cfg;
};

struct nxp_tempsense_data {
	uint16_t buffer;
	/* TCA coefficients in signed-magnitude Q11.4 stored as int16 (value * 16) */
	int16_t tca_q4[3];
	/* temperature in milli-degrees Celsius */
	int32_t temperature_mdegc;
};

static int nxp_tempsense_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct nxp_tempsense_config *config = dev->config;
	struct nxp_tempsense_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	ret = adc_read(config->adc, &config->adc_seq);
	if (ret) {
		LOG_ERR("ADC read failed (%d)", ret);
		return ret;
	}

	int32_t vets_mv_tmp = (int32_t)data->buffer;

	ret = adc_raw_to_millivolts(config->adc_ref_mv, config->ch_cfg.gain,
				    config->adc_seq.resolution, &vets_mv_tmp);
	if (ret) {
		LOG_ERR("ADC mv conversion failed (%d)", ret);
		return ret;
	}

	/* Calculate temperature in milli-degrees C using integer arithmetic.
	 * tca coefficients are in Q4 (value * 16). Formula:
	 * temp_mC = 1000*(t0/16) + (t1 * vets_mv)/16 + (t2 * vets_mv^2)/(16*1000)
	 */
	int32_t term0 = (data->tca_q4[0] * 1000) / 16;
	int32_t term1 = (data->tca_q4[1] * vets_mv_tmp) / 16;
	int64_t term2 = ((int64_t)data->tca_q4[2] * (int64_t)vets_mv_tmp *
			 (int64_t)vets_mv_tmp) / (16 * 1000);
	int64_t temp_mC = (int64_t)term0 + (int64_t)term1 + term2;

	data->temperature_mdegc = (int32_t)temp_mC;

	return 0;
}

static int nxp_tempsense_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	const struct nxp_tempsense_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	return sensor_value_from_milli(val, data->temperature_mdegc);
}

static inline int16_t tca_decode_q4(uint16_t raw)
{
	int16_t mag = (int16_t)(raw & 0x7FFFU);

	if (raw & 0x8000U) {
		return -mag;
	}

	return mag;
}

static int nxp_tempsense_init(const struct device *dev)
{
	const struct nxp_tempsense_config *config = dev->config;
	struct nxp_tempsense_data *data = dev->data;
	uint16_t tca_raw;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);

	if (ret) {
		LOG_ERR("Tempsense clock enable failed (%d)", ret);
		return ret;
	}

	if (!device_is_ready(config->adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup(config->adc, &config->ch_cfg);
	if (ret) {
		LOG_ERR("ADC channel setup failed (%d)", ret);
		return ret;
	}

	if (config->expose_ground) {
		config->base->ETSCTL |= TEMPSENSE_ETSCTL_GNDSEL_MASK;
	} else {
		config->base->ETSCTL &= ~TEMPSENSE_ETSCTL_GNDSEL_MASK;
	}

	/* Enable tempsense */
	config->base->ETSCTL |= TEMPSENSE_ETSCTL_ETS_EN_MASK;

	/* Read and decode coefficients (signed magnitude, Q11.4) */
	tca_raw = (uint16_t)(config->base->TCA0 & TEMPSENSE_TCA0_TCA0_MASK);
	data->tca_q4[0] = tca_decode_q4(tca_raw);

	tca_raw = (uint16_t)(config->base->TCA1 & TEMPSENSE_TCA1_TCA1_MASK);
	data->tca_q4[1] = tca_decode_q4(tca_raw);

	tca_raw = (uint16_t)(config->base->TCA2 & TEMPSENSE_TCA2_TCA2_MASK);
	data->tca_q4[2] = tca_decode_q4(tca_raw);

	return 0;
}

static DEVICE_API(sensor, nxp_tempsense_api) = {
	.sample_fetch = nxp_tempsense_sample_fetch,
	.channel_get = nxp_tempsense_channel_get,
};

#define NXP_TEMPSENSE_INIT(inst)								\
	static struct nxp_tempsense_data _CONCAT(nxp_tempsense_data, inst);			\
												\
	static const struct nxp_tempsense_config _CONCAT(nxp_tempsense_config, inst) = {	\
		.base = (TEMPSENSE_Type *)DT_INST_REG_ADDR(inst),				\
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),				\
		.adc_seq = {									\
			.channels = BIT(DT_INST_IO_CHANNELS_INPUT(inst)),			\
			.buffer = &_CONCAT(nxp_tempsense_data, inst).buffer,			\
			.buffer_size = sizeof(_CONCAT(nxp_tempsense_data, inst).buffer),	\
			.resolution = DT_PROP(DT_CHILD(DT_INST_IO_CHANNELS_CTLR(inst),		\
				      UTIL_CAT(channel_, DT_INST_IO_CHANNELS_INPUT(inst))),	\
				      zephyr_resolution),					\
		},										\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),				\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),	\
		.ch_cfg = ADC_CHANNEL_CFG_DT(DT_CHILD(DT_INST_IO_CHANNELS_CTLR(inst),		\
					UTIL_CAT(channel_, DT_INST_IO_CHANNELS_INPUT(inst)))),	\
		.adc_ref_mv = DT_PROP(DT_CHILD(DT_INST_IO_CHANNELS_CTLR(inst),			\
				      UTIL_CAT(channel_, DT_INST_IO_CHANNELS_INPUT(inst))),	\
				      zephyr_vref_mv),						\
		.expose_ground = DT_INST_PROP_OR(inst, nxp_expose_ground, 0),			\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, nxp_tempsense_init, NULL,				\
				     &_CONCAT(nxp_tempsense_data, inst),			\
				     &_CONCAT(nxp_tempsense_config, inst),			\
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				     &nxp_tempsense_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_TEMPSENSE_INIT)
