/*
 * Copyright (c) 2025 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_acmp

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/sensor/gecko_acmp.h>

#include <em_acmp.h>
#include <em_cmu.h>

LOG_MODULE_REGISTER(gecko_acmp, CONFIG_SENSOR_LOG_LEVEL);

#define WARMUP_TIMEOUT_MS       2U
#define CLOCK_ACMP(n)           (((n) == ACMP0) ? cmuClock_ACMP0 : ((n) == ACMP1) ? cmuClock_ACMP1 : -1)
#define GET_GECKO_ACMP_CLOCK(n) CLOCK_ACMP((ACMP_TypeDef *)DT_INST_REG_ADDR(n))
#define ACMP_IRQ_NUM(n)         (((n) == ACMP0) ? ACMP0_IRQn : ((n) == ACMP1) ? ACMP1_IRQn : -1)

const ACMP_Channel_TypeDef acmp_inputs[] = {
	acmpInputVSS,          acmpInputVREFDIVAVDD,    acmpInputVREFDIVAVDDLP,
	acmpInputVREFDIV1V25,  acmpInputVREFDIV1V25LP,  acmpInputVREFDIV2V5,
	acmpInputVREFDIV2V5LP, acmpInputVSENSE01DIV4,   acmpInputVSENSE01DIV4LP,
	acmpInputVSENSE11DIV4, acmpInputVSENSE11DIV4LP, acmpInputCAPSENSE,
};

const ACMP_Accuracy_TypeDef acmp_accuracy[] = {
	acmpAccuracyLow,
	acmpAccuracyHigh,
};

const ACMP_InputRange_TypeDef acmp_input_range[] = {
	acmpInputRangeFull,
	acmpInputRangeReduced,
};

const ACMP_HysteresisLevel_TypeDef acmp_hysteresis[] = {
	acmpHysteresisDisabled, acmpHysteresis10Sym, acmpHysteresis20Sym, acmpHysteresis30Sym,
	acmpHysteresis10Pos,    acmpHysteresis20Pos, acmpHysteresis30Pos, acmpHysteresis10Neg,
	acmpHysteresis20Neg,    acmpHysteresis30Neg,
};

enum acmp_interrupt_mode {
	ACMP_INTERRUPT_OFF,
	ACMP_INTERRUPT_EDGE_FALLING,
	ACMP_INTERRUPT_EDGE_RISING,
	ACMP_INTERRUPT_EDGE_BOTH,
};

struct gecko_acmp_config {
	ACMP_TypeDef *base;
	CMU_Clock_TypeDef clock;
#ifdef CONFIG_GECKO_ACMP_TRIGGER
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_GECKO_ACMP_TRIGGER */
#ifdef CONFIG_GECKO_ACMP_EDGE_COUNTER
	atomic_t rising_events;
	atomic_t falling_events;
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */
	uint8_t positive_input;
	uint8_t negative_input;
	uint8_t input_range;
	uint8_t accuracy;
	uint8_t hysteresis_level;
	uint32_t bias_prog;
	uint32_t vrefdiv;
	enum acmp_interrupt_mode interrupt_mode;
};

struct gecko_acmp_data {
	ACMP_Init_TypeDef acmp_config;
#ifdef CONFIG_GECKO_ACMP_TRIGGER
	const struct device *dev;
	const struct sensor_trigger *rising_trigger;
	sensor_trigger_handler_t rising_handler;
	const struct sensor_trigger *falling_trigger;
	sensor_trigger_handler_t falling_handler;
	struct k_work work;
	volatile uint32_t status;
	uint32_t interrupt_flag;
#endif /* CONFIG_GECKO_ACMP_TRIGGER */
#ifdef CONFIG_GECKO_ACMP_EDGE_COUNTER
	uint32_t rising_edge_counter;
	uint32_t falling_edge_counter;
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */
	bool cout;
};

static int gecko_acmp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct gecko_acmp_config *config = dev->config;
	struct gecko_acmp_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_ALL && (int16_t)chan != SENSOR_CHAN_GECKO_ACMP_OUTPUT
#if CONFIG_GECKO_ACMP_EDGE_COUNTER
		&& (int16_t)chan != SENSOR_CHAN_GECKO_ACMP_RISING_EDGE_COUNTER
	    && (int16_t)chan != SENSOR_CHAN_GECKO_ACMP_FALLING_EDGE_COUNTER
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */
		) {
		return -ENOTSUP;
	}

#if CONFIG_GECKO_ACMP_EDGE_COUNTER
	data->rising_edge_counter = atomic_clear(&data->rising_events);
	data->falling_edge_counter = atomic_clear(&data->falling_events);
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */

	data->cout = (config->base->STATUS & ACMP_STATUS_ACMPOUT);

	return 0;
}

static int gecko_acmp_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct gecko_acmp_data *data = dev->data;

	__ASSERT_NO_MSG(val != NULL);

	if ((int16_t)chan == SENSOR_CHAN_GECKO_ACMP_OUTPUT) {
		val->val1 = data->cout ? 1 : 0;
	}
#if CONFIG_GECKO_ACMP_EDGE_COUNTER
	else if ((int16_t)chan == SENSOR_CHAN_GECKO_ACMP_RISING_EDGE_COUNTER) {
		val->val1 = data->rising_edge_counter;
	} else if ((int16_t)chan == SENSOR_CHAN_GECKO_ACMP_FALLING_EDGE_COUNTER) {
		val->val1 = data->falling_edge_counter;
	}
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */
	else {
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_GECKO_ACMP_TRIGGER
static void gecko_acmp_isr(const struct device *dev)
{
	const struct gecko_acmp_config *config = dev->config;
	struct gecko_acmp_data *data = dev->data;

	data->status = ACMP_IntGet(config->base);

#if CONFIG_GECKO_ACMP_EDGE_COUNTER
	if (data->status & ACMP_IF_RISE) {
		atomic_inc(&data->rising_events);
	} else if (data->status & ACMP_IF_FALL) {
		atomic_inc(&data->falling_events);
	}
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */

	ACMP_IntClear(config->base, ACMP_IF_RISE | ACMP_IF_FALL);

	k_work_submit(&data->work);
}

static int gecko_acmp_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	struct gecko_acmp_data *data = dev->data;

	__ASSERT_NO_MSG(trig != NULL);

	if ((int16_t)trig->chan != SENSOR_CHAN_GECKO_ACMP_OUTPUT) {
		return -ENOTSUP;
	}

	if ((int16_t)trig->type == SENSOR_TRIG_GECKO_ACMP_OUTPUT_RISING) {
		data->rising_handler = handler;
		data->rising_trigger = trig;
	} else if ((int16_t)trig->type == SENSOR_TRIG_GECKO_ACMP_OUTPUT_FALLING) {
		data->falling_handler = handler;
		data->falling_trigger = trig;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static void gecko_acmp_trigger_work_handler(struct k_work *item)
{
	struct gecko_acmp_data *data = CONTAINER_OF(item, struct gecko_acmp_data, work);
	const struct gecko_acmp_config *config = data->dev->config;
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t handler = NULL;
	LOG_DBG("ACMP interrupt");

	if ((data->status & ACMP_IF_RISE) &&
         (!IS_ENABLED(CONFIG_GECKO_ACMP_TRIGGER_CHECK_STATE) || (config->base->STATUS & ACMP_STATUS_ACMPOUT))) {
		trigger = data->rising_trigger;
		handler = data->rising_handler;
	} else if ((data->status & ACMP_IF_FALL) &&
		 (!IS_ENABLED(CONFIG_GECKO_ACMP_TRIGGER_CHECK_STATE) || !(config->base->STATUS & ACMP_STATUS_ACMPOUT))) {
		trigger = data->falling_trigger;
		handler = data->falling_handler;
	} else {
		return;
	}

	if (handler) {
		handler(data->dev, trigger);
	}
}
#endif /* CONFIG_GECKO_ACMP_TRIGGER */

#ifdef CONFIG_PM_DEVICE

static int gecko_acmp_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct gecko_acmp_config *config = dev->config;
	struct gecko_acmp_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
#if CONFIG_GECKO_ACMP_DISABLE_ON_SUSPEND
		CMU_ClockEnable(config->clock, false);
#endif /* CONFIG_GECKO_ACMP_DISABLE_ON_SUSPEND */
#if CONFIG_GECKO_ACMP_DISABLE_INTERRUPT_ON_SUSPEND
		NVIC_DisableIRQ(ACMP_IRQ_NUM(config->base));
		ACMP_IntDisable(config->base, data->interrupt_flag);
#endif /* CONFIG_GECKO_ACMP_DISABLE_INTERRUPT_ON_SUSPEND */
		return 0;
	case PM_DEVICE_ACTION_RESUME:
#if CONFIG_GECKO_ACMP_DISABLE_ON_SUSPEND
		CMU_ClockEnable(config->clock, true);
#endif /* CONFIG_GECKO_ACMP_DISABLE_ON_SUSPEND */
#if CONFIG_GECKO_ACMP_DISABLE_INTERRUPT_ON_SUSPEND
		NVIC_EnableIRQ(ACMP_IRQ_NUM(config->base));
		ACMP_IntEnable(config->base, data->interrupt_flag);
#endif /* CONFIG_GECKO_ACMP_DISABLE_INTERRUPT_ON_SUSPEND */
		return 0;
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_TURN_OFF:
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif

static int gecko_acmp_init(const struct device *dev)
{
	const struct gecko_acmp_config *config = dev->config;
	struct gecko_acmp_data *data = dev->data;

#if defined(_SILICON_LABS_32B_SERIES_2)
	CMU_ClockSelectSet(config->clock, cmuSelect_LFRCO);
#endif

	CMU_ClockEnable(config->clock, true);

	/* Overide ACMP default configuration (hysteresis disabled) */
	data->acmp_config.enable = true;
	data->acmp_config.biasProg = config->bias_prog;
	data->acmp_config.inputRange = acmp_input_range[config->input_range];
	data->acmp_config.accuracy = acmp_accuracy[config->accuracy];
	data->acmp_config.vrefDiv = config->vrefdiv;
	data->acmp_config.hysteresisLevel = acmp_hysteresis[config->hysteresis_level];

	ACMP_Init(config->base, &data->acmp_config);

	/* Set NEGSEL input and POSSEL input */
	ACMP_ChannelSet(config->base, acmp_inputs[config->negative_input],
			acmp_inputs[config->positive_input]);

	/* Wait for warm-up */
	uint8_t timeout = WARMUP_TIMEOUT_MS;
	while (!(config->base->IF & ACMP_IF_ACMPRDY)) {
		k_sleep(K_MSEC(1));
		timeout--;
		if (timeout == 0) {
			LOG_ERR("ACMP warm-up timeout");
			return -ETIMEDOUT;
		}
	}

#ifdef CONFIG_GECKO_ACMP_TRIGGER

	switch (config->interrupt_mode) {
	case ACMP_INTERRUPT_EDGE_RISING:
		data->interrupt_flag = ACMP_IEN_RISE;
		break;
	case ACMP_INTERRUPT_EDGE_FALLING:
		data->interrupt_flag = ACMP_IEN_FALL;
		break;
	case ACMP_INTERRUPT_EDGE_BOTH:
		data->interrupt_flag = ACMP_IEN_RISE | ACMP_IEN_FALL;
		break;
	case ACMP_INTERRUPT_OFF:
	default:
		return 0;
	}

	data->dev = dev;
	k_work_init(&data->work, gecko_acmp_trigger_work_handler);
	config->irq_config_func(dev);

	/* Clear pending ACMP interrupts */
	NVIC_ClearPendingIRQ(ACMP_IRQ_NUM(config->base));
	ACMP_IntClear(config->base, data->interrupt_flag);
	/* Enable ACMP interrupts */
	NVIC_EnableIRQ(ACMP_IRQ_NUM(config->base));
	ACMP_IntEnable(config->base, data->interrupt_flag);

#endif /* CONFIG_GECKO_ACMP_TRIGGER */

	return 0;
}

static const struct sensor_driver_api gecko_acmp_driver_api = {
#ifdef CONFIG_GECKO_ACMP_TRIGGER
	.trigger_set = gecko_acmp_trigger_set,
#endif /* CONFIG_GECKO_ACMP_TRIGGER */
	.sample_fetch = gecko_acmp_sample_fetch,
	.channel_get = gecko_acmp_channel_get,
};

#define GECKO_ACMP_DECLARE_CONFIG(n, config_func_init)                                             \
	static const struct gecko_acmp_config gecko_acmp_config_##n = {                            \
		.base = (ACMP_TypeDef *)DT_INST_REG_ADDR(n),                                       \
		.bias_prog = DT_INST_PROP_OR(n, bias_level, 0),                                    \
		.vrefdiv = DT_INST_PROP_OR(n, vrefdiv, 0),                                         \
		.accuracy = DT_INST_ENUM_IDX_OR(n, accuracy, 0),                                   \
		.hysteresis_level = DT_INST_ENUM_IDX_OR(n, hysteresis_level, 0),                   \
		.input_range = DT_INST_ENUM_IDX_OR(n, input_range, 0),                             \
		.positive_input = DT_INST_ENUM_IDX(n, positive_input),                             \
		.negative_input = DT_INST_ENUM_IDX(n, negative_input),                             \
		.interrupt_mode = DT_INST_ENUM_IDX_OR(n, interrupt_mode, 3),                       \
		.clock = GET_GECKO_ACMP_CLOCK(n),                                                  \
		config_func_init}

#ifdef CONFIG_GECKO_ACMP_TRIGGER
#define GECKO_ACMP_CONFIG_FUNC(n)                                                                  \
	static void gecko_acmp_config_func_##n(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gecko_acmp_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define GECKO_ACMP_CONFIG_FUNC_INIT(n) .irq_config_func = gecko_acmp_config_func_##n
#define GECKO_ACMP_INIT_CONFIG(n)      GECKO_ACMP_DECLARE_CONFIG(n, GECKO_ACMP_CONFIG_FUNC_INIT(n))
#else /* CONFIG_GECKO_ACMP_TRIGGER */
#define GECKO_ACMP_CONFIG_FUNC(n)
#define GECKO_ACMP_CONFIG_FUNC_INIT
#define GECKO_ACMP_INIT_CONFIG(n) GECKO_ACMP_DECLARE_CONFIG(n, GECKO_ACMP_CONFIG_FUNC_INIT)
#endif /* !CONFIG_GECKO_ACMP_TRIGGER */

#define GECKO_ACMP_INIT(n)                                                                         \
	static struct gecko_acmp_data gecko_acmp_data_##n = {                                      \
		.acmp_config = ACMP_INIT_DEFAULT,                                                  \
	};                                                                                         \
	static const struct gecko_acmp_config gecko_acmp_config_##n;                               \
	PM_DEVICE_DT_INST_DEFINE(n, gecko_acmp_pm_action) \
	SENSOR_DEVICE_DT_INST_DEFINE(n, &gecko_acmp_init, PM_DEVICE_DT_INST_GET(n), &gecko_acmp_data_##n,              \
				     &gecko_acmp_config_##n, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &gecko_acmp_driver_api);         \
	GECKO_ACMP_CONFIG_FUNC(n)                                                                  \
	GECKO_ACMP_INIT_CONFIG(n);

DT_INST_FOREACH_STATUS_OKAY(GECKO_ACMP_INIT)
