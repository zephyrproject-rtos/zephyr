/*
 * Copyright (c) 2022, Prevas A/S
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_eqdc

#include <errno.h>
#include <stdint.h>

#include <fsl_eqdc.h>
#include <fsl_inputmux.h>
#include <fsl_port.h>
#include <fsl_clock.h>

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(eqdc_mcux, CONFIG_SENSOR_LOG_LEVEL);

typedef enum {
	kEqdcInputKind_P3_7_P3_8 = 0,
	kEqdcInputKind_P2_2_P2_3 = 1,
} kEqdcInputKind;

struct eqdc_mcux_config {
	EQDC_Type *base;
	const struct pinctrl_dev_config *pincfg;

	uint32_t counts_per_revolution;
	uint8_t prescaler_log2;
	kEqdcInputKind input_kind;
	uint8_t device_idx;
};

struct eqdc_mcux_data {
	int32_t position;
	float32_t speed;
};

static int eqdc_mcux_attr_set(const struct device *dev, enum sensor_channel ch,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	return -ENOTSUP;
}

static int eqdc_mcux_attr_get(const struct device *dev, enum sensor_channel ch,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	return -ENOTSUP;
}

static int eqdc_mcux_fetch(const struct device *dev, enum sensor_channel ch)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;

	if (ch != SENSOR_CHAN_ALL && ch != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	/* Calculate speed */
	if (ch == SENSOR_CHAN_ALL) {
		const int16_t pulses_since_last_read = EQDC_GetPositionDifference(config->base);
		const uint16_t ticks_between_pulses = EQDC_GetHoldPositionDifferencePeriod(config->base);
		const float32_t ticks_per_sec =
			CLOCK_GetFreq(kCLOCK_BusClk) / (1 << config->prescaler_log2);
		const float32_t pulses_per_sec_to_rpm = 60.f / (float32_t)config->counts_per_revolution;

		if (pulses_since_last_read == 0) {
			const uint16_t ticks_since_last_edge = EQDC_GetHoldLastEdgeTime(config->base);
			if (ticks_since_last_edge == UINT16_MAX) {
				/* Too long since last pulse. We consider the speed to be zero */
				data->speed = 0.f;
			} else {
				/* No pulse since last sensor fetch */

				if (ticks_since_last_edge > ticks_between_pulses) {
					/* Assume that the axle does not stop immediately,
					but rather keeps going at speeds of a fraction of a step */

					const float32_t secs_since_last_edge =
						ticks_since_last_edge / ticks_per_sec;
					if (data->speed > 0) {
						data->speed = pulses_per_sec_to_rpm / secs_since_last_edge;
					} else {
						data->speed = -pulses_per_sec_to_rpm / secs_since_last_edge;
					}
				} else {
					/* Fetched sensors just before a pulse, assume the speed is
					* kept constant */
				}
			}
		} else {
			/* At least one pulse since last sensor fetch */

			const float32_t seconds_between_pulses = (float32_t)ticks_between_pulses / ticks_per_sec;

			data->speed = (float32_t)pulses_since_last_read / seconds_between_pulses *
					pulses_per_sec_to_rpm;
		}

		LOG_DBG("POSD: %i, POSDPERH: %u", pulses_since_last_read, ticks_between_pulses);
	}

	/* Read position */
	data->position = EQDC_GetPosition(config->base);

	return 0;
}

static int eqdc_mcux_ch_get(const struct device *dev, enum sensor_channel ch,
			    struct sensor_value *val)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;

	switch (ch) {
	case SENSOR_CHAN_ROTATION:
		sensor_value_from_float(val,
					(data->position * 360.0f) / (float32_t)config->counts_per_revolution);
		break;

	case SENSOR_CHAN_RPM:
		sensor_value_from_float(val, data->speed);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, eqdc_mcux_api) = {
	.attr_set = &eqdc_mcux_attr_set,
	.attr_get = &eqdc_mcux_attr_get,
	.sample_fetch = &eqdc_mcux_fetch,
	.channel_get = &eqdc_mcux_ch_get,
};

static void init_inputs(const struct device *dev)
{
	int i;
	const struct eqdc_mcux_config *config = dev->config;

	i = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	assert(i == 0);

	/* Quadrature Encoder inputs are only accessible via inputmux */
	INPUTMUX_Init(INPUTMUX0);
	/* TODO be able to configure these via the devicetree */
	if (config->input_kind == kEqdcInputKind_P3_7_P3_8) {
		if (config->device_idx == 0) {
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn3ToQdc0Phasea);
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn2ToQdc0Phaseb);
		} else {
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn3ToQdc1Phasea);
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn2ToQdc1Phaseb);
		}
	} else if (config->input_kind == kEqdcInputKind_P2_2_P2_3) {
		if (config->device_idx == 0) {
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn6ToQdc0Phasea);
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn7ToQdc0Phaseb);
		} else {
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn6ToQdc1Phasea);
			INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_TrigIn7ToQdc1Phaseb);
		}
	}
}

static int eqdc_mcux_init(const struct device *dev)
{
	const struct eqdc_mcux_config *config = dev->config;
	eqdc_config_t eqdc_config;

	LOG_DBG("Initializing %s", dev->name);

	init_inputs(dev);

	EQDC_GetDefaultConfig(&eqdc_config);
	eqdc_config.positionModulusValue = UINT32_MAX;
	eqdc_config.prescaler = config->prescaler_log2;

	EQDC_Init(config->base, &eqdc_config);
	EQDC_SetOperateMode(config->base, kEQDC_QuadratureDecodeOperationMode);
	EQDC_DoSoftwareLoadInitialPositionValue(config->base);

	return 0;
}

#define QDEC_CHECK_COND(n, p, min, max)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, p), (				\
		    BUILD_ASSERT(IN_RANGE(DT_INST_PROP(n, p), min, max),	\
				 STRINGIFY(p) " value is out of range")), ())

#define QDEC_SET_COND(n, v, p) COND_CODE_1(DT_INST_NODE_HAS_PROP(n, p), (v = DT_INST_PROP(n, p)), ())

#define QDEC_MCUX_INIT(n)                                                                          \
                                                                                                   \
	QDEC_CHECK_COND(n, counts_per_revolution, 1, UINT32_MAX);                                  \
	QDEC_CHECK_COND(n, prescaler_log2, kEQDC_Prescaler1, kEQDC_Prescaler32768);                \
	QDEC_CHECK_COND(n, device_idx, 0, 1);                                                      \
	QDEC_CHECK_COND(n, input_kind, 0, 1);                                                      \
                                                                                                   \
	static struct eqdc_mcux_data eqdc_mcux_##n##_data = {.position = 0};                       \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct eqdc_mcux_config eqdc_mcux_##n##_config = {                            \
		.base = (EQDC_Type *)DT_INST_REG_ADDR(n),                                          \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.counts_per_revolution = DT_INST_PROP(n, counts_per_revolution),                   \
		.prescaler_log2 = DT_INST_PROP(n, prescaler_log2),                                 \
		.device_idx = DT_INST_PROP(n, device_idx),                                         \
		.input_kind = (kEqdcInputKind)DT_INST_PROP(n, input_kind)};                        \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, eqdc_mcux_init, NULL, &eqdc_mcux_##n##_data,               \
				     &eqdc_mcux_##n##_config, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &eqdc_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_MCUX_INIT)
