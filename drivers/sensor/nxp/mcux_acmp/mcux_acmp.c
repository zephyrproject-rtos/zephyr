/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright 2022, 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_acmp

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/mcux_acmp.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <fsl_acmp.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(mcux_acmp, CONFIG_SENSOR_LOG_LEVEL);

#define MCUX_ACMP_DAC_LEVELS 256
#define MCUX_ACMP_INPUT_CHANNELS 8

/*
 * Ensure the underlying MCUX definitions match the driver shim
 * assumptions. This saves us from converting between integers and
 * MCUX enumerations for sensor attributes.
 */
#if MCUX_ACMP_HAS_OFFSET
BUILD_ASSERT(kACMP_OffsetLevel0 == 0);
BUILD_ASSERT(kACMP_OffsetLevel1 == 1);
#endif /* MCUX_ACMP_HAS_OFFSET */
BUILD_ASSERT(kACMP_HysteresisLevel0 == 0);
BUILD_ASSERT(kACMP_HysteresisLevel1 == 1);
BUILD_ASSERT(kACMP_HysteresisLevel2 == 2);
BUILD_ASSERT(kACMP_HysteresisLevel3 == 3);
BUILD_ASSERT(kACMP_VrefSourceVin1 == 0);
BUILD_ASSERT(kACMP_VrefSourceVin2 == 1);
#if MCUX_ACMP_HAS_INPSEL || MCUX_ACMP_HAS_INNSEL
BUILD_ASSERT(kACMP_PortInputFromDAC == 0);
BUILD_ASSERT(kACMP_PortInputFromMux == 1);
#endif /* MCUX_ACMP_HAS_INPSEL || MCUX_ACMP_HAS_INNSEL */

struct mcux_acmp_config {
	CMP_Type *base;
	acmp_filter_config_t filter;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_MCUX_ACMP_TRIGGER
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_MCUX_ACMP_TRIGGER */
	bool high_speed : 1;
	bool unfiltered : 1;
	bool output : 1;
	bool window : 1;
};

struct mcux_acmp_data {
	acmp_config_t config;
	acmp_channel_config_t channels;
	acmp_dac_config_t dac;
#if MCUX_ACMP_HAS_DISCRETE_MODE
	acmp_discrete_mode_config_t discrete_config;
#endif
#ifdef CONFIG_MCUX_ACMP_TRIGGER
	const struct device *dev;
	sensor_trigger_handler_t rising_handler;
	const struct sensor_trigger *rising_trigger;
	sensor_trigger_handler_t falling_handler;
	const struct sensor_trigger *falling_trigger;
	struct k_work work;
	volatile uint32_t status;
#endif /* CONFIG_MCUX_ACMP_TRIGGER */
	bool cout;
};

static int mcux_acmp_attr_set(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      const struct sensor_value *val)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;
	int32_t val1;

	__ASSERT_NO_MSG(val != NULL);

	if ((int16_t)chan != SENSOR_CHAN_MCUX_ACMP_OUTPUT) {
		return -ENOTSUP;
	}

	if (val->val2 != 0) {
		return -EINVAL;
	}
	val1 = val->val1;

	switch ((int16_t)attr) {
#if MCUX_ACMP_HAS_OFFSET
	case SENSOR_ATTR_MCUX_ACMP_OFFSET_LEVEL:
		if (val1 >= kACMP_OffsetLevel0 &&
		    val1 <= kACMP_OffsetLevel1) {
			LOG_DBG("offset = %d", val1);
			data->config.offsetMode = val1;
			ACMP_Init(config->base, &data->config);
			ACMP_Enable(config->base, true);
		} else {
			return -EINVAL;
		}
		break;
#endif /* MCUX_ACMP_HAS_OFFSET */
#if MCUX_ACMP_HAS_HYSTCTR
	case SENSOR_ATTR_MCUX_ACMP_HYSTERESIS_LEVEL:
		if (val1 >= kACMP_HysteresisLevel0 &&
		    val1 <= kACMP_HysteresisLevel3) {
			LOG_DBG("hysteresis = %d", val1);
			data->config.hysteresisMode = val1;
			ACMP_Init(config->base, &data->config);
			ACMP_Enable(config->base, true);
		} else {
			return -EINVAL;
		}
		break;
#endif /* MCUX_ACMP_HAS_HYSTCTR */
	case SENSOR_ATTR_MCUX_ACMP_DAC_VOLTAGE_REFERENCE:
		if (val1 >= kACMP_VrefSourceVin1 &&
		    val1 <= kACMP_VrefSourceVin2) {
			LOG_DBG("vref = %d", val1);
			data->dac.referenceVoltageSource = val1;
			ACMP_SetDACConfig(config->base, &data->dac);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_ACMP_DAC_VALUE:
		if (val1 >= 0 && val1 < MCUX_ACMP_DAC_LEVELS) {
			LOG_DBG("dac = %d", val1);
			data->dac.DACValue = val1;
			ACMP_SetDACConfig(config->base, &data->dac);
		} else {
			return -EINVAL;
		}
		break;
#if MCUX_ACMP_HAS_INPSEL
	case SENSOR_ATTR_MCUX_ACMP_POSITIVE_PORT_INPUT:
		if (val1 >= kACMP_PortInputFromDAC &&
		    val1 <= kACMP_PortInputFromMux) {
			LOG_DBG("pport = %d", val1);
			data->channels.positivePortInput = val1;
			ACMP_SetChannelConfig(config->base, &data->channels);
		} else {
			return -EINVAL;
		}
		break;
#endif /* MCUX_ACMP_HAS_INPSEL */
	case SENSOR_ATTR_MCUX_ACMP_POSITIVE_MUX_INPUT:
		if (val1 >= 0 && val1 < MCUX_ACMP_INPUT_CHANNELS) {
			LOG_DBG("pmux = %d", val1);
			data->channels.plusMuxInput = val1;
			ACMP_SetChannelConfig(config->base, &data->channels);
		} else {
			return -EINVAL;
		}
		break;
#if MCUX_ACMP_HAS_INNSEL
	case SENSOR_ATTR_MCUX_ACMP_NEGATIVE_PORT_INPUT:
		if (val1 >= kACMP_PortInputFromDAC &&
		    val1 <= kACMP_PortInputFromMux) {
			LOG_DBG("nport = %d", val1);
			data->channels.negativePortInput = val1;
			ACMP_SetChannelConfig(config->base, &data->channels);
		} else {
			return -EINVAL;
		}
		break;
#endif /* MCUX_ACMP_HAS_INNSEL */
	case SENSOR_ATTR_MCUX_ACMP_NEGATIVE_MUX_INPUT:
		if (val1 >= 0 && val1 < MCUX_ACMP_INPUT_CHANNELS) {
			LOG_DBG("nmux = %d", val1);
			data->channels.minusMuxInput = val1;
			ACMP_SetChannelConfig(config->base, &data->channels);
		} else {
			return -EINVAL;
		}
		break;
#if MCUX_ACMP_HAS_DISCRETE_MODE
	case SENSOR_ATTR_MCUX_ACMP_POSITIVE_DISCRETE_MODE:
		if (val1 <= 1 && val1 >= 0) {
			LOG_DBG("pdiscrete = %d", val1);
			data->discrete_config.enablePositiveChannelDiscreteMode = val1;
			ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_ACMP_NEGATIVE_DISCRETE_MODE:
		if (val1 <= 1 && val1 >= 0) {
			LOG_DBG("ndiscrete = %d", val1);
			data->discrete_config.enableNegativeChannelDiscreteMode = val1;
			ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_CLOCK:
		if (val1 <= kACMP_DiscreteClockFast && val1 >= kACMP_DiscreteClockSlow) {
			LOG_DBG("discreteClk = %d", val1);
			data->discrete_config.clockSource = val1;
			ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_ENABLE_RESISTOR_DIVIDER:
		if (val1 <= 1 && val1 >= 0) {
			LOG_DBG("discreteClk = %d", val1);
			data->discrete_config.enableResistorDivider = val1;
			ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_SAMPLE_TIME:
		if (val1 <= kACMP_DiscreteSampleTimeAs256T &&
		    val1 >= kACMP_DiscreteSampleTimeAs1T) {
			LOG_DBG("discrete sampleTime = %d", val1);
			data->discrete_config.sampleTime = val1;
			ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_PHASE1_TIME:
		if (val1 <= kACMP_DiscretePhaseTimeAlt7 && val1 >= kACMP_DiscretePhaseTimeAlt0) {
			LOG_DBG("discrete phase1Time = %d", val1);
			data->discrete_config.phase1Time = val1;
			ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_PHASE2_TIME:
		if (val1 <= kACMP_DiscretePhaseTimeAlt7 && val1 >= kACMP_DiscretePhaseTimeAlt0) {
			LOG_DBG("discrete phase2Time = %d", val1);
			data->discrete_config.phase2Time = val1;
			ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
		} else {
			return -EINVAL;
		}
		break;
#endif /* MCUX_ACMP_HAS_DISCRETE_MODE */
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mcux_acmp_attr_get(const struct device *dev,
			      enum sensor_channel chan,
			      enum sensor_attribute attr,
			      struct sensor_value *val)
{
	struct mcux_acmp_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if ((int16_t)chan != SENSOR_CHAN_MCUX_ACMP_OUTPUT) {
		return -ENOTSUP;
	}

	switch ((int16_t)attr) {
#if MCUX_ACMP_HAS_OFFSET
	case SENSOR_ATTR_MCUX_ACMP_OFFSET_LEVEL:
		val->val1 = data->config.offsetMode;
		break;
#endif /* MCUX_ACMP_HAS_OFFSET */
#if MCUX_ACMP_HAS_HYSTCTR
	case SENSOR_ATTR_MCUX_ACMP_HYSTERESIS_LEVEL:
		val->val1 = data->config.hysteresisMode;
		break;
#endif /* MCUX_ACMP_HAS_HYSTCTR */
	case SENSOR_ATTR_MCUX_ACMP_DAC_VOLTAGE_REFERENCE:
		val->val1 = data->dac.referenceVoltageSource;
		break;
	case SENSOR_ATTR_MCUX_ACMP_DAC_VALUE:
		val->val1 = data->dac.DACValue;
		break;
#if MCUX_ACMP_HAS_INPSEL
	case SENSOR_ATTR_MCUX_ACMP_POSITIVE_PORT_INPUT:
		val->val1 = data->channels.positivePortInput;
		break;
#endif /* MCUX_ACMP_HAS_INPSEL */
	case SENSOR_ATTR_MCUX_ACMP_POSITIVE_MUX_INPUT:
		val->val1 = data->channels.plusMuxInput;
		break;
#if MCUX_ACMP_HAS_INNSEL
	case SENSOR_ATTR_MCUX_ACMP_NEGATIVE_PORT_INPUT:
		val->val1 = data->channels.negativePortInput;
		break;
#endif /* MCUX_ACMP_HAS_INNSEL */
	case SENSOR_ATTR_MCUX_ACMP_NEGATIVE_MUX_INPUT:
		val->val1 = data->channels.minusMuxInput;
		break;
#if MCUX_ACMP_HAS_DISCRETE_MODE
	case SENSOR_ATTR_MCUX_ACMP_POSITIVE_DISCRETE_MODE:
		val->val1 = data->discrete_config.enablePositiveChannelDiscreteMode;
		break;
	case SENSOR_ATTR_MCUX_ACMP_NEGATIVE_DISCRETE_MODE:
		val->val1 = data->discrete_config.enableNegativeChannelDiscreteMode;
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_CLOCK:
		val->val1 = data->discrete_config.clockSource;
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_ENABLE_RESISTOR_DIVIDER:
		val->val1 = data->discrete_config.enableResistorDivider;
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_SAMPLE_TIME:
		val->val1 = data->discrete_config.sampleTime;
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_PHASE1_TIME:
		val->val1 = data->discrete_config.phase1Time;
		break;
	case SENSOR_ATTR_MCUX_ACMP_DISCRETE_PHASE2_TIME:
		val->val1 = data->discrete_config.phase2Time;
		break;
#endif /* MCUX_ACMP_HAS_DISCRETE_MODE */
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static int mcux_acmp_sample_fetch(const struct device *dev,
				     enum sensor_channel chan)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;
	uint32_t status;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_ALL &&
	    (int16_t)chan != SENSOR_CHAN_MCUX_ACMP_OUTPUT) {
		return -ENOTSUP;
	}

	status = ACMP_GetStatusFlags(config->base);
	data->cout = status & kACMP_OutputAssertEventFlag;

	return 0;
}

static int mcux_acmp_channel_get(const struct device *dev,
				    enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct mcux_acmp_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if ((int16_t)chan != SENSOR_CHAN_MCUX_ACMP_OUTPUT) {
		return -ENOTSUP;
	}

	val->val1 = data->cout ? 1 : 0;
	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_MCUX_ACMP_TRIGGER
static int mcux_acmp_trigger_set(const struct device *dev,
				 const struct sensor_trigger *trig,
				 sensor_trigger_handler_t handler)
{
	struct mcux_acmp_data *data = dev->data;

	__ASSERT_NO_MSG(trig != NULL);

	if ((int16_t)trig->chan != SENSOR_CHAN_MCUX_ACMP_OUTPUT) {
		return -ENOTSUP;
	}

	switch ((int16_t)trig->type) {
	case SENSOR_TRIG_MCUX_ACMP_OUTPUT_RISING:
		data->rising_handler = handler;
		data->rising_trigger = trig;
		break;
	case SENSOR_TRIG_MCUX_ACMP_OUTPUT_FALLING:
		data->falling_handler = handler;
		data->falling_trigger = trig;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void mcux_acmp_trigger_work_handler(struct k_work *item)
{
	const struct sensor_trigger *trigger;
	struct mcux_acmp_data *data =
		CONTAINER_OF(item, struct mcux_acmp_data, work);
	sensor_trigger_handler_t handler = NULL;

	if (data->status & kACMP_OutputRisingEventFlag) {
		handler = data->rising_handler;
		trigger = data->rising_trigger;
	} else if (data->status & kACMP_OutputFallingEventFlag) {
		handler = data->falling_handler;
		trigger = data->falling_trigger;
	}

	if (handler) {
		handler(data->dev, trigger);
	}
}

static void mcux_acmp_isr(const struct device *dev)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;

	data->status = ACMP_GetStatusFlags(config->base);
	ACMP_ClearStatusFlags(config->base, data->status);

	LOG_DBG("isr status = 0x%08x", data->status);

	k_work_submit(&data->work);
}
#endif /* CONFIG_MCUX_ACMP_TRIGGER */

static int mcux_acmp_init(const struct device *dev)
{
	const struct mcux_acmp_config *config = dev->config;
	struct mcux_acmp_data *data = dev->data;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	ACMP_GetDefaultConfig(&data->config);
	data->config.enableHighSpeed = config->high_speed;
	data->config.useUnfilteredOutput = config->unfiltered;
	data->config.enablePinOut = config->output;
	ACMP_Init(config->base, &data->config);

#if MCUX_ACMP_HAS_DISCRETE_MODE
	ACMP_GetDefaultDiscreteModeConfig(&data->discrete_config);
	ACMP_SetDiscreteModeConfig(config->base, &data->discrete_config);
#endif

	ACMP_EnableWindowMode(config->base, config->window);
	ACMP_SetFilterConfig(config->base, &config->filter);
	ACMP_SetChannelConfig(config->base, &data->channels);

	/* Disable DAC */
	ACMP_SetDACConfig(config->base, NULL);

#ifdef CONFIG_MCUX_ACMP_TRIGGER
	data->dev = dev;
	k_work_init(&data->work, mcux_acmp_trigger_work_handler);

	config->irq_config_func(dev);
	ACMP_EnableInterrupts(config->base,
			      kACMP_OutputRisingInterruptEnable |
			      kACMP_OutputFallingInterruptEnable);
#endif /* CONFIG_MCUX_ACMP_TRIGGER */

	ACMP_Enable(config->base, true);

	return 0;
}

static const struct sensor_driver_api mcux_acmp_driver_api = {
	.attr_set = mcux_acmp_attr_set,
	.attr_get = mcux_acmp_attr_get,
#ifdef CONFIG_MCUX_ACMP_TRIGGER
	.trigger_set = mcux_acmp_trigger_set,
#endif /* CONFIG_MCUX_ACMP_TRIGGER */
	.sample_fetch = mcux_acmp_sample_fetch,
	.channel_get = mcux_acmp_channel_get,
};

#define MCUX_ACMP_DECLARE_CONFIG(n, config_func_init)	\
static const struct mcux_acmp_config mcux_acmp_config_##n = {		\
	.base = (CMP_Type *)DT_INST_REG_ADDR(n),			\
	.filter = {							\
		.enableSample = DT_INST_PROP(n, nxp_enable_sample),	\
		.filterCount = DT_INST_PROP_OR(n, nxp_filter_count, 0), \
		.filterPeriod = DT_INST_PROP_OR(n, nxp_filter_period, 0), \
	},								\
	.high_speed = DT_INST_PROP(n, nxp_high_speed_mode),		\
	.unfiltered = DT_INST_PROP(n, nxp_use_unfiltered_output),	\
	.output = DT_INST_PROP(n, nxp_enable_output_pin),		\
	.window = DT_INST_PROP(n, nxp_window_mode),			\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	config_func_init						\
}

#ifdef CONFIG_MCUX_ACMP_TRIGGER
#define MCUX_ACMP_CONFIG_FUNC(n)					\
	static void mcux_acmp_config_func_##n(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    mcux_acmp_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}
#define MCUX_ACMP_CONFIG_FUNC_INIT(n)					\
	.irq_config_func = mcux_acmp_config_func_##n
#define MCUX_ACMP_INIT_CONFIG(n)					\
	MCUX_ACMP_DECLARE_CONFIG(n, MCUX_ACMP_CONFIG_FUNC_INIT(n))
#else /* !CONFIG_MCUX_ACMP_TRIGGER */
#define MCUX_ACMP_CONFIG_FUNC(n)
#define MCUX_ACMP_CONFIG_FUNC_INIT
#define MCUX_ACMP_INIT_CONFIG(n)					\
	MCUX_ACMP_DECLARE_CONFIG(n, MCUX_ACMP_CONFIG_FUNC_INIT)
#endif /* !CONFIG_MCUX_ACMP_TRIGGER */

#define MCUX_ACMP_INIT(n)						\
	static struct mcux_acmp_data mcux_acmp_data_##n;		\
									\
	static const struct mcux_acmp_config mcux_acmp_config_##n;	\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	SENSOR_DEVICE_DT_INST_DEFINE(n, &mcux_acmp_init,		\
			      NULL,					\
			      &mcux_acmp_data_##n,			\
			      &mcux_acmp_config_##n, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &mcux_acmp_driver_api);			\
	MCUX_ACMP_CONFIG_FUNC(n)					\
	MCUX_ACMP_INIT_CONFIG(n);

DT_INST_FOREACH_STATUS_OKAY(MCUX_ACMP_INIT)
