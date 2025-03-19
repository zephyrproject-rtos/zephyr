/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <em_device.h>
#include <em_acmp.h>
#include <zephyr/device.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(silabs_acmp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT silabs_acmp

struct acmp_config {
	ACMP_TypeDef *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	void (*irq_init)(void);
	ACMP_Init_TypeDef init;
	int input_negative;
	int input_positive;
};

struct acmp_data {
	uint32_t interrupt_mask;
	comparator_callback_t callback;
	void *user_data;
};

static int acmp_init(const struct device *dev)
{
	int err;
	const struct acmp_config *config = dev->config;

	/* Enable ACMP Clock */
	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0 && err != -ENOENT) {
		LOG_ERR("failed to allocate silabs,analog-bus via pinctrl");
		return err;
	}

	/* Initialize the ACMP */
	ACMP_Init(config->base, &config->init);

	/* Configure the ACMP Input Channels */
	ACMP_ChannelSet(config->base, config->input_negative, config->input_positive);

	/* Initialize the irq handler */
	config->irq_init();

	return 0;
}

static int acmp_get_output(const struct device *dev)
{
	const struct acmp_config *config = dev->config;

	return config->base->STATUS & ACMP_STATUS_ACMPOUT;
}

static int acmp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct acmp_config *config = dev->config;
	struct acmp_data *data = dev->data;

	/* Disable ACMP trigger interrupts */
	ACMP_IntDisable(config->base, ACMP_IEN_RISE | ACMP_IEN_FALL);

	/* Clear ACMP trigger interrupt flags */
	ACMP_IntClear(config->base, ACMP_IEN_RISE | ACMP_IEN_FALL);

	switch (trigger) {
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		data->interrupt_mask = ACMP_IEN_RISE | ACMP_IEN_FALL;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->interrupt_mask = ACMP_IEN_RISE;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->interrupt_mask = ACMP_IEN_FALL;
		break;
	case COMPARATOR_TRIGGER_NONE:
		data->interrupt_mask = 0;
		break;
	default:
		return -EINVAL;
	}

	/* Only enable interrupts when the trigger is not none and if a
	 * callback is set.
	 */
	if (data->interrupt_mask && data->callback != NULL) {
		ACMP_IntEnable(config->base, data->interrupt_mask);
	}

	return 0;
}

static int acmp_set_trigger_callback(const struct device *dev, comparator_callback_t callback,
				     void *user_data)
{
	const struct acmp_config *config = dev->config;
	struct acmp_data *data = dev->data;

	/* Disable ACMP trigger interrupts while setting callback */
	ACMP_IntDisable(config->base, ACMP_IEN_RISE | ACMP_IEN_FALL);

	data->callback = callback;
	data->user_data = user_data;

	if (data->callback == NULL) {
		return 0;
	}

	/* Re-enable currently set ACMP trigger interrupts */
	if (data->interrupt_mask) {
		ACMP_IntEnable(config->base, data->interrupt_mask);
	}

	return 0;
}

static int acmp_trigger_is_pending(const struct device *dev)
{
	const struct acmp_config *config = dev->config;
	const struct acmp_data *data = dev->data;

	if (ACMP_IntGet(config->base) & data->interrupt_mask) {
		ACMP_IntClear(config->base, data->interrupt_mask);
		return 1;
	}

	return 0;
}

static void acmp_irq_handler(const struct device *dev)
{
	const struct acmp_config *config = dev->config;
	struct acmp_data *data = dev->data;

	ACMP_IntClear(config->base, ACMP_IF_RISE | ACMP_IF_FALL);

	if (data->callback == NULL) {
		return;
	}

	data->callback(dev, data->user_data);
}

static DEVICE_API(comparator, acmp_api) = {
	.get_output = acmp_get_output,
	.set_trigger = acmp_set_trigger,
	.set_trigger_callback = acmp_set_trigger_callback,
	.trigger_is_pending = acmp_trigger_is_pending,
};

#define ACMP_DEVICE(inst)                                                                          \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static void acmp_irq_init##inst(void)                                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), acmp_irq_handler,     \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
                                                                                                   \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
                                                                                                   \
	static struct acmp_data acmp_data##inst;                                                   \
                                                                                                   \
	static const struct acmp_config acmp_config##inst = {                                      \
		.base = (ACMP_TypeDef *)DT_INST_REG_ADDR(inst),                                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(inst),                                       \
		.irq_init = acmp_irq_init##inst,                                                   \
		.init.biasProg = DT_INST_PROP(inst, bias),                                         \
		.init.inputRange = DT_INST_ENUM_IDX(inst, input_range),                            \
		.init.accuracy = DT_INST_ENUM_IDX(inst, accuracy_mode),                            \
		.init.hysteresisLevel = DT_INST_ENUM_IDX(inst, hysteresis_mode),                   \
		.init.inactiveValue = false,                                                       \
		.init.vrefDiv = DT_INST_PROP(inst, vref_divider),                                  \
		.init.enable = true,                                                               \
		.input_negative = DT_INST_PROP(inst, input_negative),                              \
		.input_positive = DT_INST_PROP(inst, input_positive),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, acmp_init, NULL, &acmp_data##inst, &acmp_config##inst,         \
			      POST_KERNEL, CONFIG_COMPARATOR_INIT_PRIORITY, &acmp_api);

DT_INST_FOREACH_STATUS_OKAY(ACMP_DEVICE)
