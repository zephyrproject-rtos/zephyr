/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_tpm_qdec

#include <errno.h>
#include <soc.h>
#include <fsl_tpm.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/qdec_nxp_tpm.h>
#include <zephyr/logging/log.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LOG_MODULE_REGISTER(qdec_tpm, CONFIG_SENSOR_LOG_LEVEL);

struct qdec_tpm_config {
	TPM_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const tpm_phase_params_t phaseParams;
};

struct qdec_tpm_data {
	int32_t count;
	double micro_ticks_per_rev;
};

static int qdec_tpm_attr_set(const struct device *dev, enum sensor_channel ch,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	struct qdec_tpm_data *data = dev->data;

	if (ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_tpm)attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		data->micro_ticks_per_rev = val->val1 / 1000000.0f;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdec_tpm_attr_get(const struct device *dev, enum sensor_channel ch,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct qdec_tpm_data *data = dev->data;

	if (ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_tpm)attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		val->val1 = data->micro_ticks_per_rev * 1000000;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdec_tpm_fetch(const struct device *dev, enum sensor_channel ch)
{
	const struct qdec_tpm_config *config = dev->config;
	struct qdec_tpm_data *data = dev->data;

	if (ch != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* Read count */
	data->count = TPM_GetCurrentTimerCount(config->base);

	LOG_DBG("pos %d", data->count);

	return 0;
}

static int qdec_tpm_ch_get(const struct device *dev, enum sensor_channel ch,
			   struct sensor_value *val)
{
	struct qdec_tpm_data *data = dev->data;
	double rotation = (data->count * M_PI) / (data->micro_ticks_per_rev);

	switch (ch) {
	case SENSOR_CHAN_ROTATION:
		sensor_value_from_double(val, rotation);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int qdec_tpm_init(const struct device *dev)
{
	const struct qdec_tpm_config *config = dev->config;
	tpm_config_t tpm_config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	TPM_GetDefaultConfig(&tpm_config);
	tpm_config.prescale = kTPM_Prescale_Divide_1;

	TPM_Init(config->base, &tpm_config);

	TPM_SetTimerPeriod(config->base, 0xFFFFFFFF);

	TPM_SetupQuadDecode(config->base, &config->phaseParams, &config->phaseParams,
			    kTPM_QuadPhaseEncode);

	TPM_StartTimer(config->base, kTPM_SystemClock);

	return 0;
}

static DEVICE_API(sensor, qdec_tpm_api) = {
	.attr_set = &qdec_tpm_attr_set,
	.attr_get = &qdec_tpm_attr_get,
	.sample_fetch = &qdec_tpm_fetch,
	.channel_get = &qdec_tpm_ch_get,
};

#define QDEC_MCUX_INIT(n)                                                                          \
                                                                                                   \
	static struct qdec_tpm_data qdec_mcux_##n##_data = {                                       \
		.micro_ticks_per_rev =                                                             \
			(double)(DT_INST_PROP(n, micro_ticks_per_rev) / 1000000.0f)};              \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct qdec_tpm_config qdec_mcux_##n##_config = {                             \
		.base = (TPM_Type *)DT_INST_REG_ADDR(n),                                           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.phaseParams = {0, kTPM_QuadPhaseNormal}};                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, qdec_tpm_init, NULL, &qdec_mcux_##n##_data,                \
				     &qdec_mcux_##n##_config, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &qdec_tpm_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_MCUX_INIT)
