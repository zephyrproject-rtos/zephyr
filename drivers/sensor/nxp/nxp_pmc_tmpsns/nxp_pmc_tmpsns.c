/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsl_romapi_otp.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nxp_pmc_tmpsns, CONFIG_SENSOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_pmc_tmpsns

#if !CONFIG_NXP_PMC_TMPSNS_USE_FLOAT_CALC
/* Scale factor for 3 decimal places */
#define NXP_PMC_TMPSNS_TEMP_SCALE_FACTOR		1000
/* For intermediate calculations */
#define NXP_PMC_TMPSNS_TEMP_SCALE_FACTOR_LARGE		1000000

#define NXP_PMC_TMPSNS_TEMP_KELVIN_TO_CELSIUS_SCALED	273150	/* 273.15 * 1000 */
#define NXP_PMC_TMPSNS_TEMP_COEFFICIENT_SCALED		370980	/* 370.98 * 1000 */
#define NXP_PMC_TMPSNS_VREF_BASE_OFFSET_SCALED		953360	/* 953.36 * 1000 */
#define NXP_PMC_TMPSNS_VREF_SCALE_FACTOR		2048	/* Already an integer */
#define NXP_PMC_TMPSNS_CALIBRATION_MASK			0xFF
#endif

struct nxp_pmc_tmpsns_config {
	const struct device *adc;
	struct adc_sequence adc_seq;
	struct adc_channel_cfg ch_cfg;
};

struct nxp_pmc_tmpsns_data {
	uint16_t buffer;
	uint32_t pmc_tmpsns_calibration;
	float pmc_tmpsns_value;
};

#if !CONFIG_NXP_PMC_TMPSNS_USE_FLOAT_CALC
/**
 * Calculate weighted average for CTAT using integer arithmetic
 * Result is scaled by 1000 for precision.
 */
static inline int32_t calculate_cm_ctat_int(const int16_t *values)
{
	int32_t sum = (2 * values[1]  - values[2] +
		2 * values[13] - values[12] +
		2 * values[6]  - values[5] +
		2 * values[8]  - values[9]);

	return (sum * 250);
}

/**
 * Calculate weighted average for temperature using integer arithmetic
 * Result is scaled by 1000 for precision.
 */
static inline int32_t calculate_cm_temp_int(const int16_t *values)
{
	int32_t sum = (2 * values[0]  - values[3] +
		2 * values[14] - values[11] +
		4 * values[7]  - values[4] - values[10]);

	return (sum * 250);
}

/**
 * Calculate temperature in millidegrees Celsius using integer arithmetic.
 */
static int32_t get_temperature_millidegrees(struct nxp_pmc_tmpsns_data *data,
				const int16_t *pmc_tmpsns_value)
{
	if (!data || !pmc_tmpsns_value) {
		return -EINVAL;
	}

	/* Calculate temperature sensor components (scaled by 1000) */
	int32_t cm_ctat_scaled = calculate_cm_ctat_int(pmc_tmpsns_value);
	int32_t cm_temp_scaled = calculate_cm_temp_int(pmc_tmpsns_value);

	/* Extract calibration value  */
	int32_t calibration = (int8_t)(data->pmc_tmpsns_calibration &
				NXP_PMC_TMPSNS_CALIBRATION_MASK);

	/* Calculate reference voltage with calibration  */
	int64_t vref_numerator = (int64_t)(NXP_PMC_TMPSNS_VREF_BASE_OFFSET_SCALED +
			calibration * NXP_PMC_TMPSNS_TEMP_SCALE_FACTOR) * cm_temp_scaled;
	int32_t cm_vref_scaled = cm_ctat_scaled + (int32_t)(vref_numerator /
			(NXP_PMC_TMPSNS_VREF_SCALE_FACTOR * NXP_PMC_TMPSNS_TEMP_SCALE_FACTOR));

	if (cm_vref_scaled == 0) {
		return -EINVAL;
	}

	/* Calculate temperature in millidegrees Celsius */
	int64_t temp_ratio = ((int64_t)NXP_PMC_TMPSNS_TEMP_COEFFICIENT_SCALED *
					cm_temp_scaled) / cm_vref_scaled;

	return (int32_t)temp_ratio - NXP_PMC_TMPSNS_TEMP_KELVIN_TO_CELSIUS_SCALED;
}
#endif

static int nxp_pmc_tmpsns_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	uint8_t pmc_tmpsns_select[15] = {0, 1, 3, 2, 6, 7, 5, 4, 5, 7, 6, 2, 3, 1, 0};
	const struct nxp_pmc_tmpsns_config *config = dev->config;
	struct nxp_pmc_tmpsns_data *data = dev->data;
	uint16_t pmc_tmpsns_value[15] = {0};
#if CONFIG_NXP_PMC_TMPSNS_USE_FLOAT_CALC
	float cm_vref, cm_ctat, cm_temp;
	int8_t calibration = 0;
#endif
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	for (uint8_t index = 0; index < sizeof(pmc_tmpsns_select); ++index) {
		PMC0->TSENSOR = PMC_TSENSOR_TSENSM(pmc_tmpsns_select[index]);

		ret = adc_read(config->adc, &config->adc_seq);
		if (ret) {
			LOG_ERR("Failed to read ADC channels with code %d", ret);
			return ret;
		}
		pmc_tmpsns_value[index] = data->buffer;
	}

#if CONFIG_NXP_PMC_TMPSNS_USE_FLOAT_CALC
	cm_ctat = (float)(2 * pmc_tmpsns_value[1] - pmc_tmpsns_value[2] +
			2 * pmc_tmpsns_value[13] - pmc_tmpsns_value[12] +
			2 * pmc_tmpsns_value[6] - pmc_tmpsns_value[5] +
			2 * pmc_tmpsns_value[8] - pmc_tmpsns_value[9]) / 4.0f;

	cm_temp = (float)(2 * pmc_tmpsns_value[0] - pmc_tmpsns_value[3] +
			2 * pmc_tmpsns_value[14] - pmc_tmpsns_value[11] +
			4 * pmc_tmpsns_value[7] - pmc_tmpsns_value[4] -
			pmc_tmpsns_value[10]) / 4.0f;

	calibration = (int8_t)(data->pmc_tmpsns_calibration & 0xFF);

	cm_vref = cm_ctat + (953.36f + calibration) * cm_temp / 2048;

	data->pmc_tmpsns_value = 370.98f * (cm_temp / cm_vref) - 273.15f;
#else
	data->pmc_tmpsns_value = get_temperature_millidegrees(data, pmc_tmpsns_value) / 1000.0f;
#endif

	return 0;
}

static int nxp_pmc_tmpsns_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct nxp_pmc_tmpsns_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	return sensor_value_from_float(val, data->pmc_tmpsns_value);
}

static int nxp_pmc_tmpsns_init(const struct device *dev)
{
	const struct nxp_pmc_tmpsns_config *config = dev->config;
	struct nxp_pmc_tmpsns_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup(config->adc, &config->ch_cfg);
	if (ret) {
		LOG_ERR("Failed to setup ADC channel with code %d", ret);
		return ret;
	}

	ret = otp_fuse_read(CONFIG_NXP_PMC_TMPSNS_CALIBRATION_OTP_FUSE_INDEX,
			&data->pmc_tmpsns_calibration);
	if (ret) {
		LOG_ERR("Failed to get calibration value form FUSE.");
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, nxp_pmc_tmpsns_api) = {
	.sample_fetch = nxp_pmc_tmpsns_sample_fetch,
	.channel_get = nxp_pmc_tmpsns_channel_get,
};

#define NXP_PMC_TMPSNS_INIT(inst)								\
	static struct nxp_pmc_tmpsns_data _CONCAT(nxp_pmc_tmpsns_data, inst);			\
												\
	static const struct nxp_pmc_tmpsns_config _CONCAT(nxp_pmc_tmpsns_config, inst) = {	\
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),				\
		.adc_seq = {									\
			.channels = BIT(DT_INST_IO_CHANNELS_INPUT(inst)),			\
			.buffer = &_CONCAT(nxp_pmc_tmpsns_data, inst).buffer,			\
			.buffer_size = sizeof(_CONCAT(nxp_pmc_tmpsns_data, inst)),		\
			.resolution = 16,							\
			.oversampling = 7,							\
		},										\
		.ch_cfg = ADC_CHANNEL_CFG_DT(DT_CHILD(DT_INST_IO_CHANNELS_CTLR(inst),		\
				 UTIL_CAT(channel_, DT_INST_IO_CHANNELS_INPUT(inst)))),		\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, nxp_pmc_tmpsns_init, NULL,				\
				&_CONCAT(nxp_pmc_tmpsns_data, inst),				\
				&_CONCAT(nxp_pmc_tmpsns_config, inst),				\
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
				&nxp_pmc_tmpsns_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_PMC_TMPSNS_INIT)
