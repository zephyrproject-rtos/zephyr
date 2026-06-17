/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch5xx_comparator

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/comparator/comparator_wch_ch5xx.h>
#include <zephyr/dt-bindings/pinctrl/ch570-pinctrl.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <hal_ch32fun.h>

struct comparator_wch_config {
	CMP_TypeDef *regs;
	const struct pinctrl_dev_config *pin_cfg;
	void (*irq_config_func)(const struct device *dev);
};

struct comparator_wch_data {
	void *isr_data;
	comparator_callback_t isr_handler;
};

static int comparator_wch_get_output(const struct device *dev)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;

	return regs->CTRL & BIT(25) ? 1 : 0;
}

static int comparator_wch_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;
	uint32_t ctrl = regs->CTRL;

	if (trigger == COMPARATOR_TRIGGER_NONE) {
		/* Disable interrupt */
		ctrl &= ~BIT(8);
	} else {
		ctrl &= ~(0b11 << 10);
		switch (trigger) {
		case COMPARATOR_TRIGGER_RISING_EDGE:
			ctrl |= 0b11 << 10;
			break;
		case COMPARATOR_TRIGGER_FALLING_EDGE:
			ctrl |= 0b10 << 10;
			break;
		default: /* COMPARATOR_TRIGGER_BOTH */
			return -ENOTSUP;
		}

		/* Enable interrupt */
		ctrl |= BIT(8);
	}
	regs->CTRL = ctrl;

	return 0;
}

static int comparator_wch_set_nref_level(const struct device *dev, enum comp_nref_level level)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;
	uint32_t ctrl = regs->CTRL;

	ctrl &= ~(0xF << 4);
	ctrl |= (level & 0xF) << 4;

	regs->CTRL = ctrl;

	return 0;
}

static int comparator_wch_set_trigger_callback(const struct device *dev,
					       comparator_callback_t callback, void *user_data)
{
	struct comparator_wch_data *data = dev->data;

	data->isr_handler = callback;
	data->isr_data = user_data;

	return 0;
}

static int comparator_wch_trigger_is_pending(const struct device *dev)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;
	int ret = 0;

	if (regs->CTRL & BIT(16)) {
		/* Clear pending interrupt */
		regs->CTRL |= BIT(16);
		ret = 1;
	}

	return ret;
}

static void comparator_wch_isr(const struct device *dev)
{
	const struct comparator_wch_data *data = dev->data;

	if (comparator_wch_trigger_is_pending(dev)) {
		if (data->isr_handler) {
			data->isr_handler(dev, data->isr_data);
		}
	}
}

static int comparator_wch_init(const struct device *dev)
{
	const struct comparator_wch_config *config = dev->config;
	const struct pinctrl_state *pin_state;
	CMP_TypeDef *regs = config->regs;
	uint32_t ctrl = 0U;
	int err;

	/* Internal signal/pin selections */
	err = pinctrl_lookup_state(config->pin_cfg, PINCTRL_STATE_DEFAULT, &pin_state);
	if (err < 0) {
		return err;
	}

	for (int i = 0; i < pin_state->pin_cnt; i++) {
		if (pin_state->pins[i].config == CMP_P_PA7_1) {
			ctrl |= BIT(3);
		} else if (pin_state->pins[i].config == CMP_N_CMP_VREF_1) {
			ctrl |= BIT(2);
		} else {
			/* Use default configuration */
		}
	}

	/* Enable comparator */
	ctrl |= BIT(0);
	regs->CTRL = ctrl;

	err = pinctrl_apply_state_direct(config->pin_cfg, pin_state);
	if (err != 0) {
		return err;
	}
	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(comparator_wch, comparator_wch_driver_api) = {
	.parent_api = {
		.get_output = comparator_wch_get_output,
		.set_trigger = comparator_wch_set_trigger,
		.set_trigger_callback = comparator_wch_set_trigger_callback,
		.trigger_is_pending = comparator_wch_trigger_is_pending,
	},
	.set_nref_level = comparator_wch_set_nref_level,
};

#define COMPARATOR_WCH_DEFINE(idx)                                                                 \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static void comparator_wch_irq_config_func_##idx(const struct device *dev)                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), comparator_wch_isr,     \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQN(idx));                                                     \
	}                                                                                          \
	static struct comparator_wch_data comparator_wch_##idx##_data;                             \
	static const struct comparator_wch_config comparator_wch_##idx##_config = {                \
		.regs = (CMP_TypeDef *)DT_INST_REG_ADDR(idx),                                      \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                    \
		.irq_config_func = comparator_wch_irq_config_func_##idx,                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(idx, &comparator_wch_init, NULL, &comparator_wch_##idx##_data,       \
			      &comparator_wch_##idx##_config, PRE_KERNEL_1,                        \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &comparator_wch_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COMPARATOR_WCH_DEFINE)
