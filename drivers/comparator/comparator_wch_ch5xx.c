/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Documentation for this driver can be found at:
 * <https://www.wch-ic.com/downloads/CH572DS1_PDF.html> page 117.
 */

#define DT_DRV_COMPAT wch_ch5xx_comparator

#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/comparator/comparator_wch_ch5xx.h>
#include <zephyr/dt-bindings/pinctrl/ch570-pinctrl.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <hal_ch32fun.h>

/* RB_CMP_SW configurations */
#define CH5XX_CMP_SW_NREF_CFG 0b01
#define CH5XX_CMP_SW_PA7      0b10

/* RB_CMP_IF configurations */
#define CH5XX_CMP_INT_RISING_EDGE_CFG 0b11
#define CH5XX_CMP_INT_FALLING_EDGE    0b10

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

	return (regs->CTRL3 & RB_CMP_REAL_SIG) ? 1 : 0;
}

static int comparator_wch_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;
	uint8_t ctrl1 = regs->CTRL1;

	/* Disable interrupt first */
	regs->CTRL1 &= ~RB_CMP_IE;

	if (trigger == COMPARATOR_TRIGGER_NONE) {
		/* Disable interrupt */
		ctrl1 &= ~RB_CMP_IE;
	} else {
		ctrl1 &= ~RB_CMP_OUT_SEL;
		switch (trigger) {
		case COMPARATOR_TRIGGER_RISING_EDGE:
			ctrl1 |= FIELD_PREP(RB_CMP_OUT_SEL, CH5XX_CMP_INT_RISING_EDGE_CFG);
			break;
		case COMPARATOR_TRIGGER_FALLING_EDGE:
			ctrl1 |= FIELD_PREP(RB_CMP_OUT_SEL, CH5XX_CMP_INT_FALLING_EDGE);
			break;
		default:
			return -ENOTSUP;
		}

		/* Enable interrupt */
		ctrl1 |= RB_CMP_IE;
	}
	regs->CTRL1 = ctrl1;

	return 0;
}

int comparator_wch_set_nref_level(const struct device *dev, enum comp_nref_level level)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;

	regs->CTRL0 &= ~RB_CMP_NREF_LEVEL;
	regs->CTRL0 |= FIELD_PREP(RB_CMP_NREF_LEVEL, level);

	return 0;
}

static int comparator_wch_trigger_is_pending(const struct device *dev)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;

	if (regs->CTRL2 & RB_CMP_IF) {
		/* clear pendind interrupt */
		regs->CTRL2 |= RB_CMP_IF;
		return 1;
	}

	return 0;
}

static int comparator_wch_set_trigger_callback(const struct device *dev,
					       comparator_callback_t callback, void *user_data)
{
	const struct comparator_wch_config *config = dev->config;
	CMP_TypeDef *regs = config->regs;
	struct comparator_wch_data *data = dev->data;

	/* Clear and disable interrupt */
	regs->CTRL1 &= ~RB_CMP_IE;
	regs->CTRL2 |= RB_CMP_IF;

	data->isr_handler = callback;
	data->isr_data = user_data;

	/* Re-enable interrupt */
	regs->CTRL1 |= RB_CMP_IE;

	return 0;
}

static void comparator_wch_isr(const struct device *dev)
{
	const struct comparator_wch_data *data = dev->data;

	if (comparator_wch_trigger_is_pending(dev)) {
		if (data->isr_handler != NULL) {
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

	err = pinctrl_lookup_state(config->pin_cfg, PINCTRL_STATE_DEFAULT, &pin_state);
	if (err != 0) {
		return err;
	}
	for (size_t i = 0; i < pin_state->pin_cnt; i++) {
		switch (pin_state->pins[i].config) {
		case CMP_P_PA7_1:
			ctrl |= FIELD_PREP(RB_CMP_SW, CH5XX_CMP_SW_PA7);
			break;
		case CMP_N_CMP_VREF_1:
			ctrl |= FIELD_PREP(RB_CMP_SW, CH5XX_CMP_SW_NREF_CFG);
			break;
		case CMP_P_PA3_0:
		case CMP_N_PA2_0:
			/* Nothing to do */
			break;
		default:
			return -EINVAL;
		}
	}
	ctrl |= RB_CMP_EN;

	err = pinctrl_apply_state_direct(config->pin_cfg, pin_state);
	if (err != 0) {
		return err;
	}

	config->irq_config_func(dev);

	regs->CTRL = ctrl;

	return 0;
}

static DEVICE_API(comparator, comparator_wch_driver_api) = {
	.get_output = comparator_wch_get_output,
	.set_trigger = comparator_wch_set_trigger,
	.set_trigger_callback = comparator_wch_set_trigger_callback,
	.trigger_is_pending = comparator_wch_trigger_is_pending,
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
