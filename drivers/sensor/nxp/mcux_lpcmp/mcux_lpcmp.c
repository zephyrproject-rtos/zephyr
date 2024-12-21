/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpcmp

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <fsl_lpcmp.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/sensor/mcux_lpcmp.h>
#include <fsl_lpcmp.h>

LOG_MODULE_REGISTER(mcux_lpcmp, CONFIG_SENSOR_LOG_LEVEL);

struct mcux_lpcmp_config {
	LPCMP_Type *base;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_MCUX_LPCMP_TRIGGER
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */
	bool output_enable: 1;
	bool unfiltered: 1;
	bool output_invert: 1;
	lpcmp_hysteresis_mode_t hysteresis_level;
	lpcmp_power_mode_t power_level;
	lpcmp_functional_source_clock_t function_clock;
};

struct mcux_lpcmp_data {
	lpcmp_config_t lpcmp_config;
	lpcmp_dac_config_t dac_config;
	lpcmp_filter_config_t filter_config;
	lpcmp_window_control_config_t window_config;
#ifdef CONFIG_MCUX_LPCMP_TRIGGER
	const struct device *dev;
	const struct sensor_trigger *rising_trigger;
	sensor_trigger_handler_t rising_handler;
	const struct sensor_trigger *falling_trigger;
	sensor_trigger_handler_t falling_handler;
	struct k_work work;
	volatile uint32_t status;
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */
	bool cout;
};

static int mcux_lpcmp_attr_set(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct mcux_lpcmp_config *config = dev->config;
	struct mcux_lpcmp_data *data = dev->data;
	int32_t val1 = val->val1;

	__ASSERT_NO_MSG(val != NULL);

	if ((int16_t)chan != SENSOR_CHAN_MCUX_LPCMP_OUTPUT) {
		return -ENOTSUP;
	}

	if (val->val2 != 0) {
		return -EINVAL;
	}

	switch ((int16_t)attr) {
	/** Analog input mux related attributes */
	case SENSOR_ATTR_MCUX_LPCMP_POSITIVE_MUX_INPUT:
		LOG_DBG("positive mux = %d", val1);
		if (val1 >= 0 && val1 < 8) {
			config->base->CCR2 = ((config->base->CCR2 & (~LPCMP_CCR2_PSEL_MASK)) |
					      LPCMP_CCR2_PSEL(val1));
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_LPCMP_NEGATIVE_MUX_INPUT:
		LOG_DBG("negative mux = %d", val1);
		if (val1 >= 0 && val1 < 8) {
			config->base->CCR2 = ((config->base->CCR2 & (~LPCMP_CCR2_MSEL_MASK)) |
					      LPCMP_CCR2_MSEL(val1));
		} else {
			return -EINVAL;
		}
		break;

	/** DAC related attributes */
	case SENSOR_ATTR_MCUX_LPCMP_DAC_ENABLE:
		LOG_DBG("dac enable = %d", val1);
		if (val1 == 1) {
			config->base->DCR |= LPCMP_DCR_DAC_EN_MASK;
		} else if (val1 == 0) {
			config->base->DCR &= ~LPCMP_DCR_DAC_EN_MASK;
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_LPCMP_DAC_HIGH_POWER_MODE_ENABLE:
		LOG_DBG("dac power mode = %d", val1);
		if ((val1 == 1) || (val1 == 0)) {
			data->dac_config.enableLowPowerMode = (bool)val1;
			LPCMP_SetDACConfig(config->base, &data->dac_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_LPCMP_DAC_REFERENCE_VOLTAGE_SOURCE:
		LOG_DBG("dac vref = %d", val1);
		if (val1 >= kLPCMP_VrefSourceVin1 && val1 <= kLPCMP_VrefSourceVin2) {
			data->dac_config.referenceVoltageSource = val1;
			LPCMP_SetDACConfig(config->base, &data->dac_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_LPCMP_DAC_OUTPUT_VOLTAGE:
		LOG_DBG("dac value = %d", val1);
		if (val1 >= 0 && val1 < 256) {
			data->dac_config.DACValue = val1;
			LPCMP_SetDACConfig(config->base, &data->dac_config);
		} else {
			return -EINVAL;
		}
		break;

	/** Sample and filter related attributes */
	case SENSOR_ATTR_MCUX_LPCMP_SAMPLE_ENABLE:
		LOG_DBG("Filter sample enable = %d", val1);
		if ((val1 == 1) || (val1 == 0)) {
			data->filter_config.enableSample = (bool)val1;
			LPCMP_SetFilterConfig(config->base, &data->filter_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_LPCMP_FILTER_COUNT:
		LOG_DBG("sample count = %d", val1);
		data->filter_config.filterSampleCount = val1;
		LPCMP_SetFilterConfig(config->base, &data->filter_config);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_FILTER_PERIOD:
		LOG_DBG("sample period = %d", val1);
		data->filter_config.filterSamplePeriod = val1;
		LPCMP_SetFilterConfig(config->base, &data->filter_config);
		break;

	/** Window related attributes  */
	case SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_ENABLE:
		LOG_DBG("Window enable = %d", val1);
		if ((val1 == 1) || (val1 == 0)) {
			LPCMP_EnableWindowMode(config->base, (bool)val1);
		} else {
			return -EINVAL;
		}
		break;

	case SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_SIGNAL_INVERT_ENABLE:
		LOG_DBG("Invert window signal = %d", val1);
		if ((val1 == 1) || (val1 == 0)) {
			data->window_config.enableInvertWindowSignal = (bool)val1;
			LPCMP_SetWindowControl(config->base, &data->window_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_LPCMP_COUTA_SIGNAL:
		LOG_DBG("COUTA signal = %d", val1);
		if ((val1 >= (int32_t)kLPCMP_COUTASignalNoSet) &&
		    (val1 < (int32_t)kLPCMP_COUTASignalHigh)) {
			data->window_config.COUTASignal = val1;
			LPCMP_SetWindowControl(config->base, &data->window_config);
		} else {
			return -EINVAL;
		}
		break;
	case SENSOR_ATTR_MCUX_LPCMP_COUT_EVENT_TO_CLOSE_WINDOW:
		LOG_DBG("COUT event = %d", val1);
		if ((val1 >= (int32_t)kLPCMP_CLoseWindowEventNoSet) &&
		    (val1 < (int32_t)kLPCMP_CLoseWindowEventBothEdge)) {
			data->window_config.closeWindowEvent = val1;
			LPCMP_SetWindowControl(config->base, &data->window_config);
		} else {
			return -EINVAL;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mcux_lpcmp_attr_get(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	const struct mcux_lpcmp_config *config = dev->config;
	struct mcux_lpcmp_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if ((int16_t)chan != SENSOR_CHAN_MCUX_LPCMP_OUTPUT) {
		return -ENOTSUP;
	}

	switch ((int16_t)attr) {
	/** Analog mux related attributes */
	case SENSOR_ATTR_MCUX_LPCMP_POSITIVE_MUX_INPUT:
		val->val1 = (int32_t)((config->base->CCR2) &
				      LPCMP_CCR2_PSEL_MASK >> LPCMP_CCR2_PSEL_SHIFT);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_NEGATIVE_MUX_INPUT:
		val->val1 = (int32_t)((config->base->CCR2) &
				      LPCMP_CCR2_MSEL_MASK >> LPCMP_CCR2_MSEL_SHIFT);
		break;

	/** DAC related attributes */
	case SENSOR_ATTR_MCUX_LPCMP_DAC_ENABLE:
		val->val1 = (int32_t)((config->base->DCR) &
				      LPCMP_DCR_DAC_EN_MASK >> LPCMP_DCR_DAC_EN_SHIFT);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_DAC_HIGH_POWER_MODE_ENABLE:
		val->val1 = (int32_t)(data->dac_config.enableLowPowerMode);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_DAC_REFERENCE_VOLTAGE_SOURCE:
		val->val1 = (int32_t)(data->dac_config.referenceVoltageSource);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_DAC_OUTPUT_VOLTAGE:
		val->val1 = (int32_t)(data->dac_config.DACValue);
		break;

	/** Sample and filter related attributes */
	case SENSOR_ATTR_MCUX_LPCMP_SAMPLE_ENABLE:
		val->val1 = (int32_t)(data->filter_config.enableSample);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_FILTER_COUNT:
		val->val1 = (int32_t)(data->filter_config.filterSampleCount);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_FILTER_PERIOD:
		val->val1 = (int32_t)(data->filter_config.filterSamplePeriod);
		break;

	/** Window related attributes  */
	case SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_ENABLE:
		val->val1 = (int32_t)((config->base->CCR1) &
				      LPCMP_CCR1_WINDOW_EN_MASK >> LPCMP_CCR1_WINDOW_EN_SHIFT);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_SIGNAL_INVERT_ENABLE:
		val->val1 = (int32_t)(data->window_config.enableInvertWindowSignal);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_COUTA_SIGNAL:
		val->val1 = (int32_t)(data->window_config.COUTASignal);
		break;
	case SENSOR_ATTR_MCUX_LPCMP_COUT_EVENT_TO_CLOSE_WINDOW:
		val->val1 = (int32_t)(data->window_config.closeWindowEvent);
		break;

	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static int mcux_lpcmp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct mcux_lpcmp_config *config = dev->config;
	struct mcux_lpcmp_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && (int16_t)chan != SENSOR_CHAN_MCUX_LPCMP_OUTPUT) {
		return -ENOTSUP;
	}

	data->cout = ((LPCMP_GetStatusFlags(config->base) &
		       (uint32_t)kLPCMP_OutputAssertEventFlag) == kLPCMP_OutputAssertEventFlag);

	return 0;
}

static int mcux_lpcmp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct mcux_lpcmp_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if ((int16_t)chan != SENSOR_CHAN_MCUX_LPCMP_OUTPUT) {
		return -ENOTSUP;
	}

	val->val1 = data->cout ? 1 : 0;
	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_MCUX_LPCMP_TRIGGER
static void mcux_lpcmp_isr(const struct device *dev)
{
	const struct mcux_lpcmp_config *config = dev->config;
	struct mcux_lpcmp_data *data = dev->data;

	data->status = LPCMP_GetStatusFlags(config->base);
	LPCMP_ClearStatusFlags(config->base, data->status);

	k_work_submit(&data->work);
}

static int mcux_lpcmp_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	struct mcux_lpcmp_data *data = dev->data;

	__ASSERT_NO_MSG(trig != NULL);

	if ((int16_t)trig->chan != SENSOR_CHAN_MCUX_LPCMP_OUTPUT) {
		return -ENOTSUP;
	}

	if ((int16_t)trig->type == SENSOR_TRIG_MCUX_LPCMP_OUTPUT_RISING) {
		data->rising_handler = handler;
		data->rising_trigger = trig;
	} else if ((int16_t)trig->type == SENSOR_TRIG_MCUX_LPCMP_OUTPUT_FALLING) {
		data->falling_handler = handler;
		data->falling_trigger = trig;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void mcux_lpcmp_trigger_work_handler(struct k_work *item)
{
	struct mcux_lpcmp_data *data = CONTAINER_OF(item, struct mcux_lpcmp_data, work);
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t handler = NULL;

	if (((data->status & kLPCMP_OutputRisingEventFlag) == kLPCMP_OutputRisingEventFlag) &&
	    ((data->status & kLPCMP_OutputAssertEventFlag) == kLPCMP_OutputAssertEventFlag)) {
		trigger = data->rising_trigger;
		handler = data->rising_handler;
	} else if (((data->status & kLPCMP_OutputFallingEventFlag) ==
		    kLPCMP_OutputFallingEventFlag) &&
		   ((data->status & kLPCMP_OutputAssertEventFlag) !=
		    kLPCMP_OutputAssertEventFlag)) {
		trigger = data->falling_trigger;
		handler = data->falling_handler;
	} else {
		return;
	}

	if (handler) {
		handler(data->dev, trigger);
	}
}
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */

static int mcux_lpcmp_init(const struct device *dev)
{
	const struct mcux_lpcmp_config *config = dev->config;
	struct mcux_lpcmp_data *data = dev->data;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	/* LPCMP configuration */
	LPCMP_GetDefaultConfig(&data->lpcmp_config);
	data->lpcmp_config.powerMode = config->power_level;
	data->lpcmp_config.hysteresisMode = config->hysteresis_level;
	data->lpcmp_config.enableOutputPin = config->output_enable;
	data->lpcmp_config.enableInvertOutput = config->output_invert;
	data->lpcmp_config.useUnfilteredOutput = config->unfiltered;
#if defined(FSL_FEATURE_LPCMP_HAS_CCR1_FUNC_CLK_SEL) && FSL_FEATURE_LPCMP_HAS_CCR1_FUNC_CLK_SEL
	data->lpcmp_config.functionalSourceClock = config->function_clock;
#endif /* FSL_FEATURE_LPCMP_HAS_CCR1_FUNC_CLK_SEL */
	LPCMP_Init(config->base, &data->lpcmp_config);

#ifdef CONFIG_MCUX_LPCMP_TRIGGER
	data->dev = dev;
	k_work_init(&data->work, mcux_lpcmp_trigger_work_handler);
	config->irq_config_func(dev);
	LPCMP_EnableInterrupts(config->base, kLPCMP_OutputRisingInterruptEnable |
						     kLPCMP_OutputFallingInterruptEnable);
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */

	LPCMP_Enable(config->base, true);

	return 0;
}

static const struct sensor_driver_api mcux_lpcmp_driver_api = {
	.attr_set = mcux_lpcmp_attr_set,
	.attr_get = mcux_lpcmp_attr_get,
#ifdef CONFIG_MCUX_LPCMP_TRIGGER
	.trigger_set = mcux_lpcmp_trigger_set,
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */
	.sample_fetch = mcux_lpcmp_sample_fetch,
	.channel_get = mcux_lpcmp_channel_get,
};

#define MCUX_LPCMP_DECLARE_CONFIG(n, config_func_init)                                             \
	static const struct mcux_lpcmp_config mcux_lpcmp_config_##n = {                            \
		.base = (LPCMP_Type *)DT_INST_REG_ADDR(n),                                         \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.output_enable = DT_INST_PROP_OR(n, enable_output_pin, 0),                         \
		.unfiltered = DT_INST_PROP_OR(n, use_unfiltered_output, 0),                        \
		.output_invert = DT_INST_PROP_OR(n, output_invert, 0),                             \
		.hysteresis_level = DT_INST_PROP_OR(n, hysteresis_level, 0),                       \
		.power_level = DT_INST_ENUM_IDX(n, power_level),                                   \
		.function_clock = DT_INST_ENUM_IDX(n, function_clock),                             \
		config_func_init}

#ifdef CONFIG_MCUX_LPCMP_TRIGGER
#define MCUX_LPCMP_CONFIG_FUNC(n)                                                                  \
	static void mcux_lpcmp_config_func_##n(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_lpcmp_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define MCUX_LPCMP_CONFIG_FUNC_INIT(n) .irq_config_func = mcux_lpcmp_config_func_##n
#define MCUX_LPCMP_INIT_CONFIG(n)      MCUX_LPCMP_DECLARE_CONFIG(n, MCUX_LPCMP_CONFIG_FUNC_INIT(n))
#else /* CONFIG_MCUX_LPCMP_TRIGGER */
#define MCUX_LPCMP_CONFIG_FUNC(n)
#define MCUX_LPCMP_CONFIG_FUNC_INIT
#define MCUX_LPCMP_INIT_CONFIG(n) MCUX_LPCMP_DECLARE_CONFIG(n, MCUX_LPCMP_CONFIG_FUNC_INIT)
#endif /* !CONFIG_MCUX_LPCMP_TRIGGER */

#define MCUX_LPCMP_INIT(n)                                                                         \
	static struct mcux_lpcmp_data mcux_lpcmp_data_##n;                                         \
	static const struct mcux_lpcmp_config mcux_lpcmp_config_##n;                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &mcux_lpcmp_init, NULL, &mcux_lpcmp_data_##n,              \
				     &mcux_lpcmp_config_##n, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &mcux_lpcmp_driver_api);         \
	MCUX_LPCMP_CONFIG_FUNC(n)                                                                  \
	MCUX_LPCMP_INIT_CONFIG(n);

DT_INST_FOREACH_STATUS_OKAY(MCUX_LPCMP_INIT)
