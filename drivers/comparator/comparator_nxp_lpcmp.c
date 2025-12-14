/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/clock_control.h>

LOG_MODULE_REGISTER(nxp_lpcmp, CONFIG_COMPARATOR_LOG_LEVEL);

#define DT_DRV_COMPAT nxp_lpcmp

struct nxp_lpcmp_config {
	LPCMP_Type *base;
	bool enable_stop_mode;
	bool invert_output;
	bool enable_pin_out;
	bool use_unfiltered_output;
	bool positive_mux_is_dac;
	bool negative_mux_is_dac;
	uint8_t filter_count;
	uint8_t filter_period;
	uint8_t positive_mux_input;
	uint8_t negative_mux_input;
	uint8_t dac_value;
	uint8_t dac_vref_source;
	uint8_t hysteresis_mode;
	uint8_t power_mode;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
};

struct nxp_lpcmp_data {
	uint8_t interrupt_mask;
	comparator_callback_t callback;
	void *user_data;
};

static int nxp_lpcmp_get_output(const struct device *dev)
{
	const struct nxp_lpcmp_config *config = dev->config;

	return (config->base->CSR & LPCMP_CSR_COUT_MASK) ? 1 : 0;
}

static int nxp_lpcmp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct nxp_lpcmp_config *config = dev->config;
	struct nxp_lpcmp_data *data = dev->data;

	config->base->IER &= ~(LPCMP_IER_CFR_IE_MASK | LPCMP_IER_CFF_IE_MASK);
	data->interrupt_mask = 0U;

	switch (trigger) {
	case COMPARATOR_TRIGGER_NONE:
		break;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->interrupt_mask = LPCMP_IER_CFR_IE_MASK;
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->interrupt_mask = LPCMP_IER_CFF_IE_MASK;
		break;
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		data->interrupt_mask = LPCMP_IER_CFR_IE_MASK | LPCMP_IER_CFF_IE_MASK;
		break;
	default:
		LOG_ERR("Invalid trigger type.");
		return -EINVAL;
	}

	/* Clear latched status flags before enabling interrupts. */
	config->base->CSR |= (LPCMP_CSR_CFF_MASK | LPCMP_CSR_CFR_MASK);

	if ((data->interrupt_mask != 0U) && (data->callback != NULL)) {
		config->base->IER |= data->interrupt_mask;
	}

	return 0;
}

static int nxp_lpcmp_trigger_is_pending(const struct device *dev)
{
	const struct nxp_lpcmp_config *config = dev->config;
	struct nxp_lpcmp_data *data = dev->data;
	uint32_t status_flags;

	status_flags = config->base->CSR & (LPCMP_CSR_CFF_MASK | LPCMP_CSR_CFR_MASK);
	config->base->CSR |= (LPCMP_CSR_CFF_MASK | LPCMP_CSR_CFR_MASK);

	if (((data->interrupt_mask & LPCMP_IER_CFF_IE_MASK) != 0U) &&
	    ((status_flags & LPCMP_CSR_CFF_MASK) != 0U)) {
		return 1;
	}

	if (((data->interrupt_mask & LPCMP_IER_CFR_IE_MASK) != 0U) &&
	    ((status_flags & LPCMP_CSR_CFR_MASK) != 0U)) {
		return 1;
	}

	return 0;
}

static int nxp_lpcmp_set_trigger_callback(const struct device *dev, comparator_callback_t callback,
					 void *user_data)
{
	const struct nxp_lpcmp_config *config = dev->config;
	struct nxp_lpcmp_data *data = dev->data;

	config->base->CCR0 &= ~LPCMP_CCR0_CMP_EN_MASK;

	data->callback = callback;
	data->user_data = user_data;

	/* Clear any pending flags when (re)arming the callback. */
	config->base->CSR |= (LPCMP_CSR_CFF_MASK | LPCMP_CSR_CFR_MASK);

	if ((data->callback != NULL) && (data->interrupt_mask != 0U)) {
		config->base->IER |= data->interrupt_mask;
	} else {
		config->base->IER &= ~(LPCMP_IER_CFR_IE_MASK | LPCMP_IER_CFF_IE_MASK);
	}

	config->base->CCR0 |= LPCMP_CCR0_CMP_EN_MASK;

	return 0;
}

static void nxp_lpcmp_irq_handler(const struct device *dev)
{
	const struct nxp_lpcmp_config *config = dev->config;
	struct nxp_lpcmp_data *data = dev->data;

	/* Clear interrupt status flags */
	config->base->CSR |= (LPCMP_CSR_CFF_MASK | LPCMP_CSR_CFR_MASK);

	if (data->callback == NULL) {
		LOG_WRN("No callback can be executed.");
		return;
	}

	data->callback(dev, data->user_data);
}

#if CONFIG_PM_DEVICE
static int nxp_lpcmp_pm_callback(const struct device *dev, enum pm_device_action action)
{
	const struct nxp_lpcmp_config *config = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		config->base->CCR0 |= LPCMP_CCR0_CMP_EN_MASK;
		return 0;
	}

	if (action == PM_DEVICE_ACTION_SUSPEND) {
		config->base->CCR0 &= ~LPCMP_CCR0_CMP_EN_MASK;
		return 0;
	}

	return -ENOTSUP;
}
#endif

static int nxp_lpcmp_init(const struct device *dev)
{
	const struct nxp_lpcmp_config *config = dev->config;
	LPCMP_Type *base = config->base;
	int ret;

	if (config->clock_dev != NULL) {
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("Clock device is not ready");
			return -ENODEV;
		}

		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret != 0) {
			LOG_ERR("Device clock turn on failed (%d)", ret);
			return ret;
		}
	}

	/* Disable comparator before configuring. */
	base->CCR0 &= ~LPCMP_CCR0_CMP_EN_MASK;

#if defined(LPCMP_CCR0_CMP_STOP_EN_MASK)
	base->CCR0 = ((base->CCR0 & ~(LPCMP_CCR0_CMP_STOP_EN_MASK)) |
		       LPCMP_CCR0_CMP_STOP_EN(config->enable_stop_mode ? 1U : 0U));
#endif

	uint32_t ccr1 = base->CCR1;

	ccr1 = ((ccr1 & ~(LPCMP_CCR1_COUT_INV_MASK | LPCMP_CCR1_COUT_PEN_MASK |
			  LPCMP_CCR1_COUT_SEL_MASK | LPCMP_CCR1_FILT_CNT_MASK |
			  LPCMP_CCR1_FILT_PER_MASK)) |
			  LPCMP_CCR1_COUT_INV(config->invert_output ? 1U : 0U) |
			  LPCMP_CCR1_COUT_PEN(config->enable_pin_out ? 1U : 0U));

	if (config->use_unfiltered_output) {
		ccr1 |= LPCMP_CCR1_COUT_SEL_MASK;
	} else {
		ccr1 &= ~LPCMP_CCR1_COUT_SEL_MASK;
		if (config->filter_count != 0U) {
			ccr1 |= LPCMP_CCR1_FILT_CNT(config->filter_count);
			ccr1 |= LPCMP_CCR1_FILT_PER(config->filter_period);
		}
	}
	base->CCR1 = ccr1;

	uint32_t ccr2 = base->CCR2;

	ccr2 = ((ccr2 & ~(LPCMP_CCR2_CMP_HPMD_MASK | LPCMP_CCR2_HYSTCTR_MASK |
		  LPCMP_CCR2_PSEL_MASK | LPCMP_CCR2_MSEL_MASK)) |
		  LPCMP_CCR2_HYSTCTR(config->hysteresis_mode) |
		  LPCMP_CCR2_CMP_HPMD(config->power_mode == 1U));

#if defined(LPCMP_CCR2_INPSEL_MASK) && defined(LPCMP_CCR2_INMSEL_MASK)
	ccr2 &= ~(LPCMP_CCR2_INPSEL_MASK | LPCMP_CCR2_INMSEL_MASK);

	if (config->positive_mux_is_dac) {
		ccr2 |= LPCMP_CCR2_INPSEL(0U);
	} else {
		ccr2 |= (LPCMP_CCR2_INPSEL(1U) | LPCMP_CCR2_PSEL(config->positive_mux_input));
	}

	if (config->negative_mux_is_dac) {
		ccr2 |= LPCMP_CCR2_INMSEL(0U);
	} else {
		ccr2 |= (LPCMP_CCR2_INMSEL(1U) | LPCMP_CCR2_MSEL(config->negative_mux_input));
	}
#else
	uint8_t psel, msel;

	if (config->positive_mux_is_dac) {
		psel = 7U;
	} else {
		psel = config->positive_mux_input;
	}

	if (config->negative_mux_is_dac) {
		msel = 7U;
	} else {
		msel = config->negative_mux_input;
	}

	ccr2 |= (LPCMP_CCR2_PSEL(psel) | LPCMP_CCR2_MSEL(msel));
#endif

	base->CCR2 = ccr2;

	/* Configure DAC if needed. */
	base->DCR &= ~(LPCMP_DCR_DAC_EN_MASK | LPCMP_DCR_VRSEL_MASK | LPCMP_DCR_DAC_DATA_MASK);

	if (config->positive_mux_is_dac || config->negative_mux_is_dac) {
		base->DCR |= (LPCMP_DCR_VRSEL(config->dac_vref_source) |
			      LPCMP_DCR_DAC_DATA(config->dac_value) | LPCMP_DCR_DAC_EN_MASK);
	}

	/* Clear status flags before enabling interrupts or the comparator. */
	base->CSR = (LPCMP_CSR_CFF_MASK | LPCMP_CSR_CFR_MASK);
	base->IER &= ~(LPCMP_IER_CFR_IE_MASK | LPCMP_IER_CFF_IE_MASK);

	config->irq_config_func(dev);

	base->CCR0 |= LPCMP_CCR0_CMP_EN_MASK;

#if CONFIG_PM_DEVICE
	return pm_device_driver_init(dev, nxp_lpcmp_pm_callback);
#else
	return 0;
#endif
}

static DEVICE_API(comparator, nxp_lpcmp_api) = {
	.get_output = nxp_lpcmp_get_output,
	.set_trigger = nxp_lpcmp_set_trigger,
	.set_trigger_callback = nxp_lpcmp_set_trigger_callback,
	.trigger_is_pending = nxp_lpcmp_trigger_is_pending,
};

#define LPCMP_DT_ENUM_IDX(inst, prop, default_val)			\
		DT_ENUM_IDX_OR(DT_DRV_INST(inst), prop, default_val)

#define LPCMP_DT_MUX_IDX(inst, prop) LPCMP_DT_ENUM_IDX(inst, prop, 0)

#define LPCMP_DT_MUX_IS_DAC(inst, prop) DT_ENUM_HAS_VALUE(DT_DRV_INST(inst), prop, dac)

#if CONFIG_PM_DEVICE
#define LPCMP_PM_DEVICE_DEFINE PM_DEVICE_DT_INST_DEFINE(inst, nxp_lpcmp_pm_callback);
#define LPCMP_PM_DEVICE_GET PM_DEVICE_DT_INST_GET(inst)
#else
#define LPCMP_PM_DEVICE_DEFINE
#define LPCMP_PM_DEVICE_GET NULL
#endif

#define LPCMP_HAS_CLOCKS(inst) DT_INST_NODE_HAS_PROP(inst, clocks)

#define LPCMP_CLOCK_DEV(inst)									\
	COND_CODE_1(LPCMP_HAS_CLOCKS(inst), (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst))), (NULL))

#define LPCMP_CLOCK_SUBSYS(inst)								\
	COND_CODE_1(LPCMP_HAS_CLOCKS(inst),							\
		((clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name)), (NULL))

#define NXP_LPCMP_DEVICE_INIT(inst)								\
												\
	static struct nxp_lpcmp_data _CONCAT(data, inst) = {					\
		.interrupt_mask = 0U,								\
	};											\
												\
	LPCMP_PM_DEVICE_DEFINE									\
												\
	static void _CONCAT(nxp_lpcmp_irq_config, inst)(const struct device *dev)		\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),			\
			nxp_lpcmp_irq_handler, DEVICE_DT_INST_GET(inst), 0);			\
		irq_enable(DT_INST_IRQN(inst));							\
	}											\
												\
	static const struct nxp_lpcmp_config _CONCAT(config, inst) = {				\
		.base = (LPCMP_Type *)DT_INST_REG_ADDR(inst),					\
		.enable_stop_mode = DT_INST_PROP_OR(inst, enable_stop_mode, 0),			\
		.invert_output = DT_INST_PROP_OR(inst, invert_output, 0),			\
		.enable_pin_out = DT_INST_PROP_OR(inst, enable_pin_out, 0),			\
		.use_unfiltered_output = DT_INST_PROP_OR(inst, use_unfiltered_output, 0),	\
		.filter_count = DT_INST_PROP_OR(inst, filter_count, 0),				\
		.filter_period = DT_INST_PROP_OR(inst, filter_period, 0),			\
		.positive_mux_is_dac = LPCMP_DT_MUX_IS_DAC(inst, positive_mux_input),		\
		.negative_mux_is_dac = LPCMP_DT_MUX_IS_DAC(inst, negative_mux_input),		\
		.positive_mux_input = LPCMP_DT_MUX_IDX(inst, positive_mux_input),		\
		.negative_mux_input = LPCMP_DT_MUX_IDX(inst, negative_mux_input),		\
		.dac_value = DT_INST_PROP_OR(inst, dac_value, 0),				\
		.dac_vref_source = LPCMP_DT_ENUM_IDX(inst, dac_vref_source, 0),			\
		.hysteresis_mode = DT_INST_ENUM_IDX_OR(inst, hysteresis_mode, 0),		\
		.power_mode = LPCMP_DT_ENUM_IDX(inst, power_mode, 0),				\
		.clock_dev = LPCMP_CLOCK_DEV(inst),					\
		.clock_subsys = LPCMP_CLOCK_SUBSYS(inst),				\
		.irq_config_func = _CONCAT(nxp_lpcmp_irq_config, inst),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, nxp_lpcmp_init, LPCMP_PM_DEVICE_GET,			\
		&_CONCAT(data, inst), &_CONCAT(config, inst), POST_KERNEL,			\
		CONFIG_COMPARATOR_INIT_PRIORITY, &nxp_lpcmp_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_LPCMP_DEVICE_INIT)
