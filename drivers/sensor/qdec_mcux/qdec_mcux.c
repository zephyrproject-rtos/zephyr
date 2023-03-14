/*
 * Copyright (c) 2022, Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_qdec

#include <errno.h>
#include <stdint.h>

#include <fsl_enc.h>
#include <fsl_xbara.h>

#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif /* CONFIG_PINCTRL */
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/qdec_mcux.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(qdec_mcux, CONFIG_SENSOR_LOG_LEVEL);

struct qdec_mcux_config {
	ENC_Type *base;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif /* CONFIG_PINCTRL */
	XBARA_Type *xbar;
	size_t xbar_maps_len;
	int xbar_maps[];
};

struct qdec_mcux_data {
	enc_config_t qdec_config;
	int32_t position;
	uint16_t counts_per_revolution;
};

static enc_decoder_work_mode_t int_to_work_mode(int32_t val)
{
	return val == 0 ? kENC_DecoderWorkAsNormalMode :
			  kENC_DecoderWorkAsSignalPhaseCountMode;
}

static int qdec_mcux_attr_set(const struct device *dev, enum sensor_channel ch,
	enum sensor_attribute attr, const struct sensor_value *val)
{
	struct qdec_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_mcux) attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		if (!IN_RANGE(val->val1, 1, UINT16_MAX)) {
			LOG_ERR("SENSOR_ATTR_QDEC_MOD_VAL value invalid");
			return -EINVAL;
		}
		data->counts_per_revolution = val->val1;
		return 0;
	case SENSOR_ATTR_QDEC_ENABLE_SINGLE_PHASE:
		data->qdec_config.decoderWorkMode =
			int_to_work_mode(val->val1);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdec_mcux_attr_get(const struct device *dev, enum sensor_channel ch,
	enum sensor_attribute attr, struct sensor_value *val)
{
	struct qdec_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_qdec_mcux) attr) {
	case SENSOR_ATTR_QDEC_MOD_VAL:
		val->val1 = data->counts_per_revolution;
		return 0;
	case SENSOR_ATTR_QDEC_ENABLE_SINGLE_PHASE:
		val->val1 = data->qdec_config.decoderWorkMode ==
			    kENC_DecoderWorkAsNormalMode ? 0 : 1;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int qdec_mcux_fetch(const struct device *dev, enum sensor_channel ch)
{
	const struct qdec_mcux_config *config = dev->config;
	struct qdec_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* Read position */
	data->position = ENC_GetPositionValue(config->base);

	LOG_DBG("pos %d", data->position);

	return 0;
}

static int qdec_mcux_ch_get(const struct device *dev, enum sensor_channel ch,
			 struct sensor_value *val)
{
	struct qdec_mcux_data *data = dev->data;

	switch (ch) {
	case SENSOR_CHAN_ROTATION:
		val->val1 = ((int64_t)data->position * 360) /
			    data->counts_per_revolution;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api qdec_mcux_api = {
	.attr_set = &qdec_mcux_attr_set,
	.attr_get = &qdec_mcux_attr_get,
	.sample_fetch = &qdec_mcux_fetch,
	.channel_get = &qdec_mcux_ch_get,
};

static void init_inputs(const struct device *dev)
{
	int i;
	const struct qdec_mcux_config *config = dev->config;

#ifdef CONFIG_PINCTRL
	i = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	assert(i == 0);
#endif

	/* Quadrature Encoder inputs are only accessible via crossbar */
	XBARA_Init(config->xbar);
	for (i = 0; i < config->xbar_maps_len; i += 2) {
		XBARA_SetSignalsConnection(config->xbar, config->xbar_maps[i],
					   config->xbar_maps[i + 1]);
	}
}

#ifdef CONFIG_PINCTRL
#define QDEC_MCUX_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n);
#define QDEC_MCUX_PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define QDEC_MCUX_PINCTRL_DEFINE(n)
#define QDEC_MCUX_PINCTRL_INIT(n)
#endif

#define XBAR_PHANDLE(n)	DT_INST_PHANDLE(n, xbar)

#define QDEC_CHECK_COND(n, p, min, max)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, p), (				\
		    BUILD_ASSERT(IN_RANGE(DT_INST_PROP(n, p), min, max),	\
				 STRINGIFY(p) " value is out of range")), ())

#define QDEC_SET_COND(n, v, p)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, p), (v = DT_INST_PROP(n, p)), ())

#define QDEC_MCUX_INIT(n)							\
										\
	BUILD_ASSERT((DT_PROP_LEN(XBAR_PHANDLE(n), xbar_maps) % 2) == 0,	\
			"xbar_maps length must be an even number");		\
	QDEC_CHECK_COND(n, counts_per_revolution, 1, UINT16_MAX);		\
	QDEC_CHECK_COND(n, filter_sample_period, 0, UINT8_MAX);			\
										\
	static struct qdec_mcux_data qdec_mcux_##n##_data = {			\
		.counts_per_revolution = DT_INST_PROP(n, counts_per_revolution) \
	};									\
										\
	QDEC_MCUX_PINCTRL_DEFINE(n)						\
										\
	static const struct qdec_mcux_config qdec_mcux_##n##_config = {		\
		.base = (ENC_Type *)DT_INST_REG_ADDR(n),			\
		.xbar = (XBARA_Type *)DT_REG_ADDR(XBAR_PHANDLE(n)),		\
		.xbar_maps_len = DT_PROP_LEN(XBAR_PHANDLE(n), xbar_maps),	\
		.xbar_maps = DT_PROP(XBAR_PHANDLE(n), xbar_maps),		\
		QDEC_MCUX_PINCTRL_INIT(n)					\
	};									\
										\
	static int qdec_mcux_##n##_init(const struct device *dev)		\
	{									\
		const struct qdec_mcux_config *config = dev->config;		\
		struct qdec_mcux_data *data = dev->data;			\
										\
		LOG_DBG("Initializing %s", dev->name);				\
										\
		init_inputs(dev);						\
										\
		ENC_GetDefaultConfig(&data->qdec_config);			\
		data->qdec_config.decoderWorkMode = int_to_work_mode(		\
			DT_INST_PROP(n, single_phase_mode));			\
		QDEC_SET_COND(n, data->qdec_config.filterCount, filter_count);	\
		QDEC_SET_COND(n, data->qdec_config.filterSamplePeriod,		\
			  filter_sample_period);				\
		LOG_DBG("Latency is %u filter clock cycles + 2 IPBus clock "	\
			"periods", data->qdec_config.filterSamplePeriod *	\
			(data->qdec_config.filterCount + 3));			\
		ENC_Init(config->base, &data->qdec_config);			\
										\
		/* Update the position counter with initial value. */		\
		ENC_DoSoftwareLoadInitialPositionValue(config->base);		\
										\
		return 0;							\
	}									\
										\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(n, qdec_mcux_##n##_init, NULL,		\
			      &qdec_mcux_##n##_data, &qdec_mcux_##n##_config,	\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
			      &qdec_mcux_api);					\
										\

DT_INST_FOREACH_STATUS_OKAY(QDEC_MCUX_INIT)
