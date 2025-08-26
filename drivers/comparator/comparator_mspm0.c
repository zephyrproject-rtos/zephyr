/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_comparator

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/irq.h>
#include <ti/driverlib/dl_comp.h>


struct comparator_mspm0_ref_config {
	DL_COMP_REF_SOURCE source;
	DL_COMP_REF_TERMINAL_SELECT terminal;
	DL_COMP_DAC_CONTROL dac_control;
	DL_COMP_DAC_INPUT dac_input;
	uint8_t dac_code0;
	uint8_t dac_code1;
};

struct comparator_mspm0_config {
	DL_COMP_IPSEL_CHANNEL pos_amux_ch;
	DL_COMP_IMSEL_CHANNEL neg_amux_ch;
	DL_COMP_MODE mode;
	DL_COMP_HYSTERESIS hysteresis;
	struct comparator_mspm0_ref_config ref_config;
	DL_COMP_FILTER_DELAY filter_delay;
	COMP_Regs *regs;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
	const struct device *ref;
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	DL_COMP_IMSEL_CHANNEL window_lower_thresh;
	COMP_Regs *window_companion_regs;
	bool window_mode_enable;
#endif
	bool filter_enable;
};

struct comparator_mspm0_data {
	void *user_data;
	comparator_callback_t callback;
	struct k_mutex dev_lock;
};

static int comparator_mspm0_get_output(const struct device *dev)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->dev_lock, K_FOREVER);
	ret = DL_COMP_getComparatorOutput(config->regs);
	k_mutex_unlock(&data->dev_lock);

	return ret;
}

static int comparator_mspm0_set_trigger(const struct device *dev,
					enum comparator_trigger trigger)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;
	uint32_t interrupt_mask = 0;
	int ret = 0;

	k_mutex_lock(&data->dev_lock, K_FOREVER);
	DL_COMP_disableInterrupt(config->regs,
				 DL_COMP_INTERRUPT_OUTPUT_EDGE |
				 DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);

	if (trigger == COMPARATOR_TRIGGER_NONE) {
		DL_COMP_clearInterruptStatus(config->regs,
					     DL_COMP_INTERRUPT_OUTPUT_EDGE |
					     DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);
		goto out;
	}
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	if (config->window_mode_enable) {
		DL_COMP_setOutputInterruptEdge(config->regs,
					       DL_COMP_OUTPUT_INT_EDGE_RISING |
					       DL_COMP_OUTPUT_INT_EDGE_FALLING);

		interrupt_mask = DL_COMP_INTERRUPT_OUTPUT_EDGE |
				 DL_COMP_INTERRUPT_OUTPUT_EDGE_INV;

	} else {
#endif
		switch (trigger) {
		case COMPARATOR_TRIGGER_RISING_EDGE:
			DL_COMP_setOutputInterruptEdge(config->regs,
						       DL_COMP_OUTPUT_INT_EDGE_RISING);
			interrupt_mask = DL_COMP_INTERRUPT_OUTPUT_EDGE;
			break;

		case COMPARATOR_TRIGGER_FALLING_EDGE:
			DL_COMP_setOutputInterruptEdge(config->regs,
						       DL_COMP_OUTPUT_INT_EDGE_FALLING);
			interrupt_mask = DL_COMP_INTERRUPT_OUTPUT_EDGE;
			break;

		case COMPARATOR_TRIGGER_BOTH_EDGES:
			DL_COMP_setOutputInterruptEdge(config->regs,
						       DL_COMP_OUTPUT_INT_EDGE_RISING |
						       DL_COMP_OUTPUT_INT_EDGE_FALLING);
			interrupt_mask = DL_COMP_INTERRUPT_OUTPUT_EDGE |
					 DL_COMP_INTERRUPT_OUTPUT_EDGE_INV;
			break;

		default:
			ret = -EINVAL;
			goto out;
		}
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	}
#endif

	DL_COMP_enableInterrupt(config->regs, interrupt_mask);
out:
	k_mutex_unlock(&data->dev_lock);
	return ret;
}

static int comparator_mspm0_set_trigger_callback(const struct device *dev,
						 comparator_callback_t callback,
						 void *user_data)
{
	struct comparator_mspm0_data *data = dev->data;

	k_mutex_lock(&data->dev_lock, K_FOREVER);
	data->callback = callback;
	data->user_data = user_data;
	k_mutex_unlock(&data->dev_lock);

	return 0;
}

static int comparator_mspm0_trigger_is_pending(const struct device *dev)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;
	int pending;

	k_mutex_lock(&data->dev_lock, K_FOREVER);
	pending = DL_COMP_getEnabledInterruptStatus(config->regs,
						    DL_COMP_INTERRUPT_OUTPUT_EDGE |
						    DL_COMP_INTERRUPT_OUTPUT_EDGE_INV);
	if (pending) {
		DL_COMP_clearInterruptStatus(config->regs, pending);
		k_mutex_unlock(&data->dev_lock);
		return 1;
	}
	k_mutex_unlock(&data->dev_lock);
	return 0;
}

static void comparator_mspm0_isr(const struct device *dev)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;

	if (DL_COMP_getPendingInterrupt(config->regs) && data->callback) {
		data->callback(dev, data->user_data);
	}
}

static int comparator_mspm0_init(const struct device *dev)
{
	const struct comparator_mspm0_config *config = dev->config;
	struct comparator_mspm0_data *data = dev->data;
	DL_COMP_Config comp_config = {
		.mode = config->mode,
		.channelEnable = DL_COMP_ENABLE_CHANNEL_POS_NEG,
		.posChannel = config->pos_amux_ch,
		.negChannel = config->neg_amux_ch,
		.polarity = DL_COMP_POLARITY_NON_INV,
		.hysteresis = config->hysteresis
	};

	DL_COMP_RefVoltageConfig reference_config = {
		.mode = DL_COMP_REF_MODE_STATIC,
		.source = config->ref_config.source,
		.terminalSelect = config->ref_config.terminal,
		.controlSelect = config->ref_config.dac_control,
		.inputSelect = config->ref_config.dac_input
	};

	k_mutex_init(&data->dev_lock);
	DL_COMP_enablePower(config->regs);
	if (!DL_COMP_isPowerEnabled(config->regs)) {
		return -EIO;
	}

	pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	DL_COMP_init(config->regs, &comp_config);

	if ((config->mode == DL_COMP_MODE_FAST) && config->filter_enable) {
		DL_COMP_enableOutputFilter(config->regs, config->filter_delay);
	}
	if (config->ref_config.source != DL_COMP_REF_SOURCE_NONE) {
		DL_COMP_refVoltageInit(config->regs, &reference_config);
		DL_COMP_setDACCode0(config->regs, config->ref_config.dac_code0);
		DL_COMP_setDACCode1(config->regs, config->ref_config.dac_code1);
		if (config->ref_config.terminal == DL_COMP_REF_TERMINAL_SELECT_NEG) {
		/* neg reference terminal is selected then only enable positive channel selection */
			DL_COMP_setEnabledInputChannels(config->regs,
							DL_COMP_ENABLE_CHANNEL_POS);
		} else {
		/* pos reference terminal is selected then only enable negative channel selection */
			DL_COMP_setEnabledInputChannels(config->regs,
							DL_COMP_ENABLE_CHANNEL_NEG);
		}

		DL_COMP_setReferenceCompTerminal(config->regs,
						 config->ref_config.terminal);
#ifdef CONFIG_REGULATOR_MSPM0_VREF
		if (config->ref &&
		    ((config->ref_config.source == DL_COMP_REF_SOURCE_INT_VREF_DAC) ||
		    (config->ref_config.source == DL_COMP_REF_SOURCE_INT_VREF))) {
			if (regulator_enable(config->ref) < 0) {
				return -ENODEV;
			}
		}
#endif
	}
#ifdef CONFIG_COMPARATOR_MSPM0_WINDOW_MODE
	if (config->window_mode_enable && config->window_companion_regs) {
		DL_COMP_enableWindowComparator(config->regs);
		/* If pos ref terminal is selected it will be used for window comparator input
		 * otherwise positive channel input is used
		 */
		if ((config->ref_config.source == DL_COMP_REF_SOURCE_NONE) ||
		    (config->ref_config.terminal == DL_COMP_REF_TERMINAL_SELECT_NEG)) {
			DL_COMP_setPositiveChannelInput(config->regs,
							config->pos_amux_ch);
		}

		/* If neg ref terminal is selected  will be used for window comparator upper
		 * threshold input otherwise negative  channel selected is used
		 */

		if ((config->ref_config.source == DL_COMP_REF_SOURCE_NONE) ||
		    (config->ref_config.terminal == DL_COMP_REF_TERMINAL_SELECT_POS)) {
			DL_COMP_setNegativeChannelInput(config->regs,
							config->neg_amux_ch);
		}

		DL_COMP_disableWindowComparator(config->window_companion_regs);
		DL_COMP_setPositiveChannelInput(config->window_companion_regs,
						DL_COMP_IPSEL_CHANNEL_7);
		DL_COMP_setNegativeChannelInput(config->window_companion_regs,
						config->window_lower_thresh);
		DL_COMP_enable(config->window_companion_regs);
	}
#endif
	config->irq_config_func(dev);
	DL_COMP_enable(config->regs);

	return 0;
}

static DEVICE_API(comparator, comparator_mspm0_api) = {
	.get_output = comparator_mspm0_get_output,
	.set_trigger = comparator_mspm0_set_trigger,
	.set_trigger_callback = comparator_mspm0_set_trigger_callback,
	.trigger_is_pending = comparator_mspm0_trigger_is_pending,
};

#define COMPARATOR_MSPM0_DEFINE(n)							\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static void comparator_mspm0_irq_config_##n(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    comparator_mspm0_isr,					\
			    DEVICE_DT_INST_GET(n), 0);					\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static const struct comparator_mspm0_config comparator_mspm0_config_##n = {	\
	.regs = (COMP_Regs *)DT_INST_REG_ADDR(n),					\
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
	.pos_amux_ch = _CONCAT(DL_COMP_IPSEL_CHANNEL_,					\
			DT_INST_PROP_OR(n, positive_inputs, 0)),			\
	.neg_amux_ch = _CONCAT(DL_COMP_IMSEL_CHANNEL_,					\
			DT_INST_PROP_OR(n, negative_inputs, 0)),			\
	.mode = _CONCAT(DL_COMP_MODE_,							\
			DT_INST_STRING_UPPER_TOKEN_OR(n, ti_mode, FAST)),		\
	.hysteresis = _CONCAT(DL_COMP_HYSTERESIS_,					\
			DT_INST_STRING_UPPER_TOKEN_OR(n, ti_hysteresis, NONE)),		\
	.ref_config = {									\
		.source = _CONCAT(DL_COMP_REF_SOURCE_,					\
				DT_INST_STRING_UPPER_TOKEN_OR(n,			\
				ti_reference_source, NONE)),				\
		.terminal = _CONCAT(DL_COMP_REF_TERMINAL_SELECT_,			\
				DT_INST_STRING_UPPER_TOKEN_OR(n,			\
				ti_reference_terminal, NEG)),				\
		.dac_code0 = DT_INST_PROP_OR(n, ti_reference_dac_code0, 128),		\
		.dac_code1 = DT_INST_PROP_OR(n, ti_reference_dac_code1, 128),		\
		.dac_control = _CONCAT(DL_COMP_DAC_CONTROL_,				\
				DT_INST_STRING_UPPER_TOKEN_OR(n,			\
				ti_reference_dac_control, COMP_OUT)),			\
		.dac_input = _CONCAT(DL_COMP_DAC_INPUT_DACCODE,				\
				DT_INST_PROP_OR(n, ti_reference_dac_input, 0))		\
	},										\
	.filter_enable = DT_INST_PROP_OR(n, ti_filter_enable, false),			\
	.filter_delay = _CONCAT(DL_COMP_FILTER_DELAY_,					\
			DT_INST_PROP_OR(n, ti_filter_delay, 70)),			\
	IF_ENABLED(CONFIG_COMPARATOR_MSPM0_WINDOW_MODE,					\
	(.window_mode_enable = DT_INST_PROP_OR(n, ti_window_mode_enable, false),	\
	 .window_companion_regs = COND_CODE_1(						\
				DT_INST_NODE_HAS_PROP(n, ti_window_companion),		\
				((COMP_Regs *)DT_REG_ADDR(DT_PHANDLE(DT_DRV_INST(n),	\
				ti_window_companion))), (NULL)),			\
	.window_lower_thresh = _CONCAT(DL_COMP_IMSEL_CHANNEL_,				\
				DT_INST_PROP_OR(n, ti_window_lower_threshold, 0)),))	\
	.irq_config_func = comparator_mspm0_irq_config_##n,				\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, vref),					\
	(.ref = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), vref)),),			\
	(.ref = NULL))									\
	};										\
											\
	static struct comparator_mspm0_data comparator_mspm0_data_##n;			\
											\
	DEVICE_DT_INST_DEFINE(n,							\
		comparator_mspm0_init,							\
		NULL,									\
		&comparator_mspm0_data_##n,						\
		&comparator_mspm0_config_##n,						\
		POST_KERNEL,								\
		CONFIG_COMPARATOR_INIT_PRIORITY,					\
		&comparator_mspm0_api);							\

DT_INST_FOREACH_STATUS_OKAY(COMPARATOR_MSPM0_DEFINE)
