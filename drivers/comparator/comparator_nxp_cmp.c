/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>

LOG_MODULE_REGISTER(nxp_cmp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_cmp

struct nxp_cmp_config {
	CMP_Type *base;
	bool enable_high_speed_mode;
	bool invert_output;
	bool enable_pin_out;
	bool use_unfiltered_output;
	uint8_t filter_count;
	uint8_t filter_period;
	uint8_t positive_mux_input;
	uint8_t negative_mux_input;
	uint8_t dac_value;
	uint8_t dac_vref_source;
	uint8_t hysteresis_mode;
	void (*irq_config_func)(const struct device *dev);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct nxp_cmp_data {
	uint8_t interrupt_mask;
	comparator_callback_t callback;
	void *user_data;
};

static int nxp_cmp_get_output(const struct device *dev)
{
	const struct nxp_cmp_config *config = dev->config;

	return ((bool)(config->base->SCR & CMP_SCR_COUT_MASK)) ? 1 : 0;
}

static int nxp_cmp_set_trigger(const struct device *dev,
			enum comparator_trigger trigger)
{
	const struct nxp_cmp_config *config = dev->config;
	struct nxp_cmp_data *data = dev->data;

	config->base->SCR &= ~(CMP_SCR_IEF_MASK | CMP_SCR_IER_MASK);

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		data->interrupt_mask = 0;
		break;

	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->interrupt_mask = CMP_SCR_IER_MASK;
		break;

	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->interrupt_mask = CMP_SCR_IEF_MASK;
		break;

	case COMPARATOR_TRIGGER_BOTH_EDGES:
		data->interrupt_mask = CMP_SCR_IEF_MASK | CMP_SCR_IER_MASK;
		break;

	default:
		LOG_ERR("Invalid trigger type.");
		return -EINVAL;
	}

	/* Enable interrupts when interrupt_mask is non-zero and
	 * callback function is not NULL.
	 */
	if ((data->interrupt_mask != 0) && (data->callback != NULL)) {
		config->base->SCR |= data->interrupt_mask;
	}

	return 0;
}

static int nxp_cmp_trigger_is_pending(const struct device *dev)
{
	const struct nxp_cmp_config *config = dev->config;
	struct nxp_cmp_data *data = dev->data;
	uint8_t status_flags;

	status_flags = config->base->SCR & (CMP_SCR_CFF_MASK | CMP_SCR_CFR_MASK);
	config->base->SCR |= (CMP_SCR_CFF_MASK | CMP_SCR_CFR_MASK);

	if (((data->interrupt_mask & CMP_SCR_IEF_MASK) != 0) &&
		((status_flags & CMP_SCR_CFF_MASK) != 0)) {
		return 1;
	}

	if (((data->interrupt_mask & CMP_SCR_IER_MASK) != 0) &&
		((status_flags & CMP_SCR_CFR_MASK) != 0)) {
		return 1;
	}

	return 0;
}

static int nxp_cmp_set_trigger_callback(const struct device *dev,
					comparator_callback_t callback,
					void *user_data)
{
	const struct nxp_cmp_config *config = dev->config;
	struct nxp_cmp_data *data = dev->data;

	config->base->CR1 &= ~CMP_CR1_EN_MASK;

	data->callback = callback;
	data->user_data = user_data;

	config->base->CR1 |= CMP_CR1_EN_MASK;

	if (data->callback == NULL) {
		LOG_INF("Callback is not set.");
		return 0;
	}

	return 0;
}

static void nxp_cmp_irq_handler(const struct device *dev)
{
	const struct nxp_cmp_config *config = dev->config;
	struct nxp_cmp_data *data = dev->data;

	/* Clear interrupt status flags */
	config->base->SCR |= (CMP_SCR_CFF_MASK | CMP_SCR_CFR_MASK);

	if (data->callback == NULL) {
		LOG_WRN("No callback can be executed.");
		return;
	}

	data->callback(dev, data->user_data);
}

static int nxp_cmp_pm_callback(const struct device *dev,
			enum pm_device_action action)
{
	const struct nxp_cmp_config *config = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		config->base->CR1 |= CMP_CR1_EN_MASK;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		config->base->CR1 &= ~CMP_CR1_EN_MASK;
	}

	return 0;
}

static int nxp_cmp_init(const struct device *dev)
{
	const struct nxp_cmp_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);

	if (ret) {
		LOG_ERR("Device clock turn on failed");
		return ret;
	}

	config->base->CR0 = ((config->base->CR0 & ~CMP_CR0_HYSTCTR_MASK) |
			CMP_CR0_HYSTCTR(config->hysteresis_mode));

	config->base->CR1 = ((config->base->CR1 & ~(CMP_CR1_PMODE_MASK |
		CMP_CR1_INV_MASK | CMP_CR1_OPE_MASK | CMP_CR1_COS_MASK)) |
		(CMP_CR1_PMODE(config->enable_high_speed_mode) |
		CMP_CR1_INV(config->invert_output) |
		CMP_CR1_OPE(config->enable_pin_out) |
		CMP_CR1_COS(config->use_unfiltered_output)));

	/* Input mux configuration */
	config->base->MUXCR = ((config->base->MUXCR & ~(CMP_MUXCR_PSEL_MASK |
		CMP_MUXCR_MSEL_MASK)) | (CMP_MUXCR_PSEL(config->positive_mux_input) |
		CMP_MUXCR_MSEL(config->negative_mux_input)));

	/* DAC configuration */
	if (7 == config->negative_mux_input) {
		config->base->DACCR = ((config->base->DACCR & ~(CMP_DACCR_VRSEL_MASK |
			CMP_DACCR_VOSEL_MASK)) | (CMP_DACCR_VRSEL(config->dac_vref_source) |
			CMP_DACCR_VOSEL(config->dac_value) | CMP_DACCR_DACEN_MASK));
	}

	/* Filter configuration. */
	if (0 != config->filter_count) {
		config->base->CR1 &= ~CMP_CR1_SE_MASK;
		config->base->FPR = CMP_FPR_FILT_PER(config->filter_period);
		config->base->CR0 = ((config->base->CR0 & ~CMP_CR0_FILTER_CNT_MASK) |
				CMP_CR0_FILTER_CNT(config->filter_count));
	}

	config->irq_config_func(dev);

	return pm_device_driver_init(dev, nxp_cmp_pm_callback);
}

static DEVICE_API(comparator, nxp_cmp_api) = {
	.get_output = nxp_cmp_get_output,
	.set_trigger = nxp_cmp_set_trigger,
	.set_trigger_callback = nxp_cmp_set_trigger_callback,
	.trigger_is_pending = nxp_cmp_trigger_is_pending,
};

#define NXP_CMP_DEVICE_INIT(inst)						\
										\
	static struct nxp_cmp_data _CONCAT(data, inst) = {			\
		.interrupt_mask = 0,						\
	};									\
										\
	PM_DEVICE_DT_INST_DEFINE(inst, nxp_cmp_pm_callback);			\
										\
	static void _CONCAT(nxp_cmp_irq_config, inst)(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	\
			nxp_cmp_irq_handler, DEVICE_DT_INST_GET(inst), 0);	\
		irq_enable(DT_INST_IRQN(inst));					\
	}									\
										\
	static const struct nxp_cmp_config _CONCAT(config, inst) = {		\
		.base = (CMP_Type *)DT_INST_REG_ADDR(inst),			\
		.enable_high_speed_mode = DT_INST_PROP(inst,			\
					enable_high_speed_mode),		\
		.invert_output = DT_INST_PROP(inst, invert_output),		\
		.enable_pin_out = DT_INST_PROP(inst, enable_pin_out),		\
		.use_unfiltered_output = DT_INST_PROP(inst,			\
					use_unfiltered_output),			\
		.filter_count = DT_INST_PROP_OR(inst, filter_count, 0),		\
		.filter_period = DT_INST_PROP_OR(inst, filter_period, 0),	\
		.positive_mux_input = DT_ENUM_IDX_OR(DT_DRV_INST(inst),		\
				positive_mux_input, 0),				\
		.negative_mux_input = DT_ENUM_IDX_OR(DT_DRV_INST(inst),		\
				negative_mux_input, 0),				\
		.dac_value = DT_INST_PROP_OR(inst, dac_value, 0),		\
		.dac_vref_source = DT_ENUM_IDX_OR(DT_DRV_INST(inst),		\
				dac_vref_source, 0),				\
		.hysteresis_mode = DT_INST_PROP_OR(DT_DRV_INST(inst),		\
				hysteresis_mode, 0),				\
		.irq_config_func = _CONCAT(nxp_cmp_irq_config, inst),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),		\
		.clock_subsys =							\
			(clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, nxp_cmp_init, PM_DEVICE_DT_INST_GET(inst),	\
			&_CONCAT(data, inst), &_CONCAT(config, inst),		\
			POST_KERNEL, CONFIG_COMPARATOR_INIT_PRIORITY,		\
			&nxp_cmp_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_CMP_DEVICE_INIT)
