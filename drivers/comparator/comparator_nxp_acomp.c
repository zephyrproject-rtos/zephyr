/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(nxp_acomp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_acomp

#define ACOMP_NEG_INPUT_VIO_0P25 12U

struct nxp_acomp_config {
	ACOMP_Type *base;
	bool async_output;
	bool invert_output;
	bool enable_pin_out;
	bool inactive_value_high;
	uint8_t positive_input;
	uint8_t negative_input;
	uint8_t positive_hysteresis;
	uint8_t negative_hysteresis;
	uint8_t warmup_time_us;
	uint8_t response_mode;
	void (*irq_config_func)(const struct device *dev);
};

struct nxp_acomp_data {
	comparator_callback_t callback;
	void *user_data;
	uint32_t interrupt_mask;
};

static int nxp_acomp_get_output(const struct device *dev)
{
	const struct nxp_acomp_config *config = dev->config;
	uint32_t status = config->base->STATUS0;

	return ((bool)(status & ACOMP_STATUS0_OUT_MASK)) ? 1U : 0U;
}

static int nxp_acomp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct nxp_acomp_config *config = dev->config;
	struct nxp_acomp_data *data = dev->data;
	uint32_t ctrl = config->base->CTRL0;

	ctrl &= ~(ACOMP_CTRL0_INT_ACT_HI_MASK | ACOMP_CTRL0_EDGE_LEVL_SEL_MASK);

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		data->interrupt_mask = 0U;
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		ctrl |= (ACOMP_CTRL0_EDGE_LEVL_SEL(1U) | ACOMP_CTRL0_INT_ACT_HI(1U));
		data->interrupt_mask = ACOMP_IMR0_OUTA_INT_MASK_MASK | ACOMP_IMR0_OUT_INT_MASK_MASK;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		ctrl |= (ACOMP_CTRL0_EDGE_LEVL_SEL(1U) | ACOMP_CTRL0_INT_ACT_HI(0U));
		data->interrupt_mask = ACOMP_IMR0_OUTA_INT_MASK_MASK | ACOMP_IMR0_OUT_INT_MASK_MASK;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		LOG_ERR("Trigger type not support.");
		return -ENOTSUP;
	default:
		LOG_ERR("Trigger type invalid.");
		return -EINVAL;
	}

	config->base->CTRL0 = ctrl;

	/* Clear latched status flags before enabling interrupts. */
	config->base->ICR0 = ACOMP_ICR0_OUT_INT_CLR_MASK | ACOMP_ICR0_OUTA_INT_CLR_MASK;

	if ((data->interrupt_mask != 0U) && (data->callback != NULL)) {
		config->base->IMR0 &= ~data->interrupt_mask;
	} else {
		config->base->IMR0 |= data->interrupt_mask;
	}

	return 0;
}

static int nxp_acomp_trigger_is_pending(const struct device *dev)
{
	const struct nxp_acomp_config *config = dev->config;
	struct nxp_acomp_data *data = dev->data;

	/* Read raw status first. We'll clear the hardware flag when we
	 * consume the pending event to avoid leaving sticky RAW bits set
	 * and producing repeated reports.
	 */
	bool interrupts_enabled = (data->interrupt_mask & (ACOMP_IMR0_OUTA_INT_MASK_MASK |
				ACOMP_IMR0_OUT_INT_MASK_MASK)) != 0U;
	bool interrupt_flags = ((config->base->IRSR0 & (ACOMP_IRSR0_OUTA_INT_RAW_MASK |
				ACOMP_IRSR0_OUT_INT_RAW_MASK))) != 0U;
	int pending = (interrupts_enabled && interrupt_flags) ? 1 : 0;

	/* Always clear raw flags so we don't re-report the same edge. */
	config->base->ICR0 = ACOMP_ICR0_OUTA_INT_CLR_MASK | ACOMP_ICR0_OUT_INT_CLR_MASK;

	return pending;
}

static int nxp_acomp_set_trigger_callback(const struct device *dev,
				comparator_callback_t callback, void *user_data)
{
	const struct nxp_acomp_config *config = dev->config;
	struct nxp_acomp_data *data = dev->data;

	config->base->CTRL0 &= ~ACOMP_CTRL0_EN_MASK;

	data->callback = callback;
	data->user_data = user_data;

	/* Clear any pending flags when (re)arming the callback. */
	config->base->ICR0 = ACOMP_ICR0_OUTA_INT_CLR_MASK | ACOMP_ICR0_OUT_INT_CLR_MASK;

	if ((data->callback != NULL) && (data->interrupt_mask != 0U)) {
		config->base->IMR0 &= ~data->interrupt_mask;
	} else {
		config->base->IMR0 |= data->interrupt_mask;
	}

	config->base->CTRL0 |= ACOMP_CTRL0_EN_MASK;

	return 0;
}

static void nxp_acomp_irq_handler(const struct device *dev)
{
	const struct nxp_acomp_config *config = dev->config;
	struct nxp_acomp_data *data = dev->data;
	uint32_t status = config->base->IRSR0;
	uint32_t raw_mask = ACOMP_IRSR0_OUTA_INT_RAW_MASK | ACOMP_IRSR0_OUT_INT_RAW_MASK;

	/* Clear interrupt status flags */
	config->base->ICR0 = ACOMP_ICR0_OUTA_INT_CLR_MASK | ACOMP_ICR0_OUT_INT_CLR_MASK;

	if ((status & raw_mask) == 0U) {
		return;
	}

	if (data->callback == NULL) {
		LOG_WRN("No callback can be executed.");
		return;
	}

	/* RAW bits stay asserted; mask both sources after one hit to avoid ISR storms. */
	config->base->IMR0 |= ACOMP_IMR0_OUTA_INT_MASK_MASK | ACOMP_IMR0_OUT_INT_MASK_MASK;

	data->callback(dev, data->user_data);
}

#if CONFIG_PM_DEVICE
static int nxp_acomp_pm_callback(const struct device *dev,
			enum pm_device_action action)
{
	const struct nxp_acomp_config *config = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		config->base->CTRL0 |= ACOMP_CTRL0_EN_MASK;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		config->base->CTRL0 &= ~ACOMP_CTRL0_EN_MASK;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static int nxp_acomp_init(const struct device *dev)
{
	const struct nxp_acomp_config *config = dev->config;
	uint32_t ctrl = config->base->CTRL0;
	uint32_t route = config->base->ROUTE0;

	/* Do software first. */
	config->base->RST0 |= ACOMP_RST0_SOFT_RST_MASK;
	config->base->RST0 &= ~ACOMP_RST0_SOFT_RST_MASK;

	ctrl &= ~(ACOMP_CTRL0_WARMTIME_MASK | ACOMP_CTRL0_BIAS_PROG_MASK |
			ACOMP_CTRL0_INACT_VAL_MASK | ACOMP_CTRL0_GPIOINV_MASK |
			ACOMP_CTRL0_HYST_SELP_MASK | ACOMP_CTRL0_HYST_SELN_MASK |
			ACOMP_CTRL0_POS_SEL_MASK | ACOMP_CTRL0_NEG_SEL_MASK |
			ACOMP_CTRL0_LEVEL_SEL_MASK | ACOMP_CTRL0_MUXEN_MASK);

	ctrl |= ACOMP_CTRL0_WARMTIME(config->warmup_time_us) |
		ACOMP_CTRL0_BIAS_PROG(config->response_mode) |
		ACOMP_CTRL0_INACT_VAL(config->inactive_value_high) |
		ACOMP_CTRL0_GPIOINV(config->invert_output) |
		ACOMP_CTRL0_HYST_SELP(config->positive_hysteresis) |
		ACOMP_CTRL0_HYST_SELN(config->negative_hysteresis) |
		ACOMP_CTRL0_POS_SEL(config->positive_input) |
		ACOMP_CTRL0_NEG_SEL(config->negative_input) |
		ACOMP_CTRL0_MUXEN(1U);

	/* VIO-based negative inputs use LEVEL_SEL[1:0] to pick 0.25/0.5/0.75/1.0. */
	if (config->negative_input >= ACOMP_NEG_INPUT_VIO_0P25) {
		uint32_t level = (config->negative_input - ACOMP_NEG_INPUT_VIO_0P25) & 0x3U;

		ctrl |= ACOMP_CTRL0_LEVEL_SEL(level);
	}
	config->base->CTRL0 = ctrl;

	route &= ~(ACOMP_ROUTE0_OUTSEL_MASK | ACOMP_ROUTE0_PE_MASK);
	route |= ACOMP_ROUTE0_OUTSEL(config->async_output) |
		ACOMP_ROUTE0_PE(config->enable_pin_out);
	config->base->ROUTE0 = route;

	/* Disable interrupt, clear status. */
	config->base->IMR0 |= ACOMP_IMR0_OUT_INT_MASK_MASK | ACOMP_IMR0_OUTA_INT_MASK_MASK;
	config->base->ICR0 = ACOMP_ICR0_OUT_INT_CLR_MASK | ACOMP_ICR0_OUTA_INT_CLR_MASK;

	config->base->CTRL0 |= ACOMP_CTRL0_EN_MASK;

	config->irq_config_func(dev);

#if CONFIG_PM_DEVICE
	return pm_device_driver_init(dev, nxp_acomp_pm_callback);
#else
	return 0;
#endif
}

static DEVICE_API(comparator, nxp_acomp_api) = {
	.get_output = nxp_acomp_get_output,
	.set_trigger = nxp_acomp_set_trigger,
	.set_trigger_callback = nxp_acomp_set_trigger_callback,
	.trigger_is_pending = nxp_acomp_trigger_is_pending,
};

#if CONFIG_PM_DEVICE
#define ACOMP_PM_DEVICE_DEFINE	PM_DEVICE_DT_INST_DEFINE(inst, nxp_acomp_pm_callback);
#define ACOMP_PM_DEVICE_GET	PM_DEVICE_DT_INST_GET(inst)
#else
#define ACOMP_PM_DEVICE_DEFINE
#define ACOMP_PM_DEVICE_GET	NULL
#endif

#define NXP_ACOMP_INIT(inst)									\
	ACOMP_PM_DEVICE_DEFINE									\
												\
	static void nxp_acomp_irq_config_##inst(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),			\
			    nxp_acomp_irq_handler, DEVICE_DT_INST_GET(inst), 0);		\
		irq_enable(DT_INST_IRQN(inst));							\
	}											\
												\
	static struct nxp_acomp_data nxp_acomp_data_##inst;					\
												\
	static const struct nxp_acomp_config nxp_acomp_config_##inst = {			\
		.base = (ACOMP_Type *)DT_INST_REG_ADDR(inst),					\
		.positive_input = DT_ENUM_IDX(DT_DRV_INST(inst), positive_input),		\
		.negative_input = DT_ENUM_IDX(DT_DRV_INST(inst), negative_input),		\
		.positive_hysteresis = DT_INST_PROP_OR(inst, positive_hysteresis_mv / 10, 0),	\
		.negative_hysteresis = DT_INST_PROP_OR(inst, negative_hysteresis_mv / 10, 0),	\
		.warmup_time_us = DT_INST_PROP_OR(inst, warmup_time_us, 0),			\
		.response_mode = DT_ENUM_IDX_OR(DT_DRV_INST(inst), response_mode, 0),		\
		.inactive_value_high = DT_INST_PROP_OR(inst, inactive_value_high, 0),		\
		.invert_output = DT_INST_PROP_OR(inst, invert_output, 0),			\
		.enable_pin_out = DT_INST_PROP_OR(inst, enable_pin_out, 0),			\
		.async_output = DT_INST_PROP_OR(inst, async_output, 0),				\
		.irq_config_func = nxp_acomp_irq_config_##inst,					\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, nxp_acomp_init, NULL,					\
		&nxp_acomp_data_##inst, &nxp_acomp_config_##inst, POST_KERNEL,			\
		CONFIG_COMPARATOR_INIT_PRIORITY, &nxp_acomp_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_ACOMP_INIT)
