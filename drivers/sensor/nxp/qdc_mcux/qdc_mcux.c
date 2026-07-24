/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_qdc

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <fsl_qdc.h>

#include <zephyr/drivers/mux.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/qdec_mcux.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(qdc_mcux, CONFIG_SENSOR_LOG_LEVEL);

struct qdc_mux_entry {
	const struct device *dev;
	const struct mux_state *state;
};

struct qdc_mcux_config {
	QDC_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct qdc_mux_entry *mux_entries;
	uint8_t mux_entries_len;
	uint8_t filter_count;
	uint8_t filter_sample_period;
	bool single_phase_mode;
};

struct qdc_mcux_data {
	qdc_config_t qdc_config;
	int32_t position;
	uint32_t counts_per_revolution;
};

static qdc_decoder_work_mode_t int_to_work_mode(int32_t val)
{
	return val == 0 ? kQDC_DecoderWorkAsNormalMode :
			  kQDC_DecoderWorkAsSignalPhaseCountMode;
}

static int qdc_mcux_attr_set(const struct device *dev, enum sensor_channel ch,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct qdc_mcux_config *config = dev->config;
	struct qdc_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_mcux)attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		if (!IN_RANGE(val->val1, 1, UINT32_MAX)) {
			LOG_ERR("SENSOR_ATTR_QDEC_MOD_VAL value invalid");
			return -EINVAL;
		}
		data->counts_per_revolution = val->val1;
		data->qdc_config.positionModulusValue = data->counts_per_revolution - 1U;
		config->base->UMOD = (uint16_t)(data->qdc_config.positionModulusValue >> 16U);
		config->base->LMOD = (uint16_t)data->qdc_config.positionModulusValue;
		return 0;
	case SENSOR_ATTR_QDEC_ENABLE_SINGLE_PHASE:
		data->qdc_config.decoderWorkMode = int_to_work_mode(val->val1);
		WRITE_BIT(config->base->CTRL, QDC_CTRL_PH1_SHIFT, val->val1);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdc_mcux_attr_get(const struct device *dev, enum sensor_channel ch,
			     enum sensor_attribute attr, struct sensor_value *val)
{
	struct qdc_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_mcux)attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		val->val1 = data->counts_per_revolution;
		return 0;
	case SENSOR_ATTR_QDEC_ENABLE_SINGLE_PHASE:
		val->val1 = data->qdc_config.decoderWorkMode ==
				    kQDC_DecoderWorkAsNormalMode ?
			    0 :
			    1;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdc_mcux_fetch(const struct device *dev, enum sensor_channel ch)
{
	const struct qdc_mcux_config *config = dev->config;
	struct qdc_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	data->position = (int32_t)QDC_GetPositionValue(config->base);

	LOG_DBG("pos %d", data->position);

	return 0;
}

static int qdc_mcux_ch_get(const struct device *dev, enum sensor_channel ch,
			   struct sensor_value *val)
{
	struct qdc_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	sensor_value_from_float(val, (data->position * 360.0f) /
						 data->counts_per_revolution);

	return 0;
}

static DEVICE_API(sensor, qdc_mcux_api) = {
	.attr_set = &qdc_mcux_attr_set,
	.attr_get = &qdc_mcux_attr_get,
	.sample_fetch = &qdc_mcux_fetch,
	.channel_get = &qdc_mcux_ch_get,
};

static int qdc_mcux_init(const struct device *dev)
{
	const struct qdc_mcux_config *config = dev->config;
	struct qdc_mcux_data *data = dev->data;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0 && err != -ENOENT) {
		return err;
	}

	for (uint8_t i = 0; i < config->mux_entries_len; i++) {
		const struct qdc_mux_entry *e = &config->mux_entries[i];

		if (!device_is_ready(e->dev)) {
			LOG_ERR("mux controller %s not ready", e->dev->name);
			return -ENODEV;
		}

		err = mux_state_apply(e->dev, e->state);
		if (err) {
			LOG_ERR("Failed to apply mux state %d: %d", i, err);
			return err;
		}
	}

	QDC_GetDefaultConfig(&data->qdc_config);
	data->qdc_config.decoderWorkMode = int_to_work_mode(config->single_phase_mode);
	data->qdc_config.filterCount = config->filter_count;
	data->qdc_config.filterSamplePeriod = config->filter_sample_period;
	data->qdc_config.enableModuloCountMode = true;
	data->qdc_config.positionModulusValue = data->counts_per_revolution - 1U;
	QDC_Init(config->base, &data->qdc_config);
	QDC_DoSoftwareLoadInitialPositionValue(config->base);

	return 0;
}

#define QDC_CHECK_COND(n, p, min, max)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, p), (				\
		    BUILD_ASSERT(IN_RANGE(DT_INST_PROP(n, p), min, max),	\
				 STRINGIFY(p) " value is out of range")), ())

#define QDC_MUX_ENTRY_INIT(node_id, prop, idx)					\
	{									\
		.dev   = MUX_STATE_DT_DEV_GET_BY_IDX(node_id, idx),		\
		.state = MUX_STATE_DT_GET_BY_IDX(node_id, idx),			\
	},

#define QDC_MCUX_INIT(n)							\
	QDC_CHECK_COND(n, filter_count, 0, 7);					\
	BUILD_ASSERT(DT_INST_NODE_HAS_PROP(n, mux_states),			\
		     "nxp,mcux-qdc node " #n				        \
		     " requires a mux-states property to route external "	\
		     "signals into the QDC inputs");				\
										\
	static struct qdc_mcux_data qdc_mcux_##n##_data = {			\
		.counts_per_revolution = DT_INST_PROP(n, counts_per_revolution), \
	};									\
										\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	MUX_STATE_DT_INST_SPEC_DEFINE_ALL(n);					\
	static const struct qdc_mux_entry qdc_mcux_##n##_mux_entries[] = {	\
		DT_INST_FOREACH_PROP_ELEM(n, mux_states, QDC_MUX_ENTRY_INIT)	\
	};									\
										\
	static const struct qdc_mcux_config qdc_mcux_##n##_config = {		\
		.base = (QDC_Type *)DT_INST_REG_ADDR(n),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.filter_count = DT_INST_PROP_OR(n, filter_count, 0),		\
		.filter_sample_period = DT_INST_PROP_OR(n, filter_sample_period, 0), \
		.single_phase_mode = DT_INST_PROP(n, single_phase_mode),	\
		.mux_entries = qdc_mcux_##n##_mux_entries,			\
		.mux_entries_len = ARRAY_SIZE(qdc_mcux_##n##_mux_entries),	\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(n, qdc_mcux_init, NULL,			\
				     &qdc_mcux_##n##_data,			\
				     &qdc_mcux_##n##_config, POST_KERNEL,	\
				     CONFIG_SENSOR_INIT_PRIORITY, &qdc_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(QDC_MCUX_INIT)
