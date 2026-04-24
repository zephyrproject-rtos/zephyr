/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include <IfxEgtm_reg.h>
#include <IfxScu_reg.h>
#include <soc.h>

#include "gpio_tc4x.h"

#define DT_DRV_COMPAT infineon_tc4x_gpio

/**
 * @brief Common gpio flags to custom flags
 */
static int gpio_tc4x_flags_to_drvcfg(gpio_flags_t flags, Ifx_P_PADCFG_DRVCFG *drvcfg)
{
	bool is_input = flags & GPIO_INPUT;
	bool is_output = flags & GPIO_OUTPUT;

	/* Disconnect not supported */
	if (!is_input && !is_output) {
		return -ENOTSUP;
	}

	/* Open source not supported*/
	if (flags & GPIO_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	/* Pull up & pull down not supported in output mode */
	if (is_output && (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	drvcfg->B.PL = 0;

	if (is_input) {
		drvcfg->B.DIR = 0;
		if (flags & (GPIO_PULL_UP)) {
			drvcfg->B.MODE = GPIO_TC4X_INPUT_PULL_UP;
		} else if (flags & (GPIO_PULL_DOWN)) {
			drvcfg->B.MODE = GPIO_TC4X_INPUT_PULL_DOWN;
		} else {
			drvcfg->B.MODE = GPIO_TC4X_INPUT_GPIO;
		}
	}
	if (is_output) {
		drvcfg->B.DIR = 1;
		drvcfg->B.MODE = 0;
		drvcfg->B.PD = 0;
		if (flags & GPIO_OPEN_DRAIN) {
			drvcfg->B.OD = 1;
		} else {
			drvcfg->B.OD = 0;
		}
	}

	return 0;
}

static int gpio_tc4x_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_tc4x_config *cfg = dev->config;

	*value = cfg->base->IN.U;

	return 0;
}

static int gpio_tc4x_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct gpio_tc4x_config *cfg = dev->config;
	uint32_t clear, set;

	mask &= 0xFFFF;
	value &= 0xFFFF;

	set = (mask & value);
	clear = (mask & ~value);

	cfg->base->OMR.U = (clear << 16) | set;

	return 0;
}

static int gpio_tc4x_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc4x_config *cfg = dev->config;

	cfg->base->OMSR.U = 0xFFFF & pins;

	return 0;
}

static int gpio_tc4x_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc4x_config *cfg = dev->config;

	cfg->base->OMCR.U = (0xFFFF & pins) << 16;

	return 0;
}

static int gpio_tc4x_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc4x_config *cfg = dev->config;
	uint32_t out;
	uint64_t swap;

	do {
		out = cfg->base->OUT.U;
		swap = ((uint64_t)out << 32) | (out ^ pins);
		__asm("	cmpswap.w [%1]+0, %A0\n" : "+d"(swap) : "a"(&cfg->base->OUT));
	} while ((swap & 0xFFFFFFFF) != out);

	return 0;
}

/**
 * @brief Configure pin or port
 */
static int gpio_tc4x_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_tc4x_config *cfg = dev->config;
	int err;
	Ifx_P_PADCFG_DRVCFG drvcfg;

	/* figure out if we can map the requested GPIO
	 * configuration
	 */
	err = gpio_tc4x_flags_to_drvcfg(flags, &drvcfg);
	if (err != 0) {
		return err;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_tc4x_port_set_bits_raw(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_tc4x_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	volatile void *accrgp_prote = &cfg->base->ACCGRP[cfg->access_group].PROTE;
	bool init = aurix_prot_get_state(accrgp_prote) == AURIX_PROT_STATE_INIT;

	if (!init) {
		aurix_prot_set_state(accrgp_prote, AURIX_PROT_STATE_CONFIG);
	}
	cfg->base->PADCFG[pin].DRVCFG = drvcfg;
	if (!init) {
		aurix_prot_set_state(accrgp_prote, AURIX_PROT_STATE_RUN);
	}

	return 0;
}

struct gpio_tc4x_irq_tim {
	uint8_t tim;
	uint8_t ch;
	uint8_t mux;
};

static int gpio_tc4x_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_tc4x_config *cfg = dev->config;
	const struct gpio_tc4x_irq_source *irq_src = NULL;
	uint32_t i = 0;

	for (i = 0; i < cfg->irq_source_count; i++) {
		if (cfg->irq_sources[i].pin == pin) {
			irq_src = &cfg->irq_sources[i];
		}
	}

	if (irq_src == NULL) {
		return -ENOTSUP;
	}

	if (irq_src->type == TC4X_IRQ_TYPE_GTM) {
		if (MODULE_EGTM.CLC.B.DISS == 1) {
			return -EIO;
		}

		Ifx_EGTM_CLS_TIM_CH_CTRL ctrl = {.U = 0};
		Ifx_EGTM_CLS_TIM_CH_IRQ_EN irq_en = {.B.NEWVAL_IRQ_EN = 1};

		ctrl.B.TIM_EN = 1;
		ctrl.B.CLK_SEL = 7;
		ctrl.B.FLT_CNT_FRQ = 0x3;
		ctrl.B.TIM_MODE = mode == GPIO_INT_MODE_EDGE ? 0x2 : 0x5;
		ctrl.B.DSL = trig == GPIO_INT_TRIG_HIGH ? 1 : 0;
		ctrl.B.ISL = trig == GPIO_INT_TRIG_BOTH ? 1 : 0;
		ctrl.B.FLT_EN = 1;
		ctrl.B.FLT_MODE_FE = mode == GPIO_INT_MODE_LEVEL ? 0x1 : 0;
		ctrl.B.FLT_MODE_RE = mode == GPIO_INT_MODE_LEVEL ? 0x1 : 0;

		MODULE_EGTM.TIMINSEL[irq_src->cls].U =
			(MODULE_EGTM.TIMINSEL[irq_src->cls].U & ~(0xF << (4 * irq_src->ch))) |
			(irq_src->mux << (4 * irq_src->ch));
		MODULE_EGTM.CLS[irq_src->cls].TIM.CH[irq_src->ch].CTRL = ctrl;
		MODULE_EGTM.CLS[irq_src->cls].TIM.CH[irq_src->ch].CNTS.U = 10;
		MODULE_EGTM.CLS[irq_src->cls].TIM.CH[irq_src->ch].FLT_FE.U = 0x5;
		MODULE_EGTM.CLS[irq_src->cls].TIM.CH[irq_src->ch].FLT_RE.U = 0x5;
		MODULE_EGTM.CLS[irq_src->cls].TIM.CH[irq_src->ch].IRQ_EN = irq_en;

		irq_enable((0x1E60 / 4 + irq_src->cls * 8 + irq_src->ch));

		return 0;
	}

	if (irq_src->type == TC4X_IRQ_TYPE_ERU) {
		Ifx_SCU_ERU_EICR eicr = {.U = 0};
		Ifx_SCU_ERU_IGCR igcr = {.U = 0};

		eicr.B.EIEN = mode != GPIO_INT_MODE_DISABLED;
		eicr.B.EIEN_P = 1;
		eicr.B.EISEL = irq_src->mux;
		eicr.B.ONP = irq_src->cls;
		eicr.B.TPEN = 1;
		eicr.B.REN = (trig & GPIO_INT_TRIG_HIGH) != 0;
		eicr.B.FEN = (trig & GPIO_INT_TRIG_LOW) != 0;
		eicr.B.LDEN = (mode == GPIO_INT_MODE_LEVEL);
		MODULE_SCU.ERU.EICR[irq_src->ch] = eicr;

		/* We use the masking of the irq via the INTFx to emulate a
		 * level type irq behavior. The irq is retriggered from the ISR
		 * as long as the level is present.
		 */
		igcr.U = mode == GPIO_INT_MODE_LEVEL ? BIT(irq_src->ch) : 0;
		igcr.B.IGP = mode == GPIO_INT_MODE_EDGE ? 1 : 2;
		MODULE_SCU.ERU.IGCR[irq_src->cls] = igcr;

		return 0;
	}

	return -ENOTSUP;
}

static int gpio_tc4x_manage_callback(const struct device *dev, struct gpio_callback *callback,
				      bool set)
{
	struct gpio_tc4x_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(gpio, tc4x_gpio_driver_api) = {
	.pin_configure = gpio_tc4x_config,
	.port_get_raw = gpio_tc4x_port_get_raw,
	.port_set_masked_raw = gpio_tc4x_port_set_masked_raw,
	.port_set_bits_raw = gpio_tc4x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tc4x_port_clear_bits_raw,
	.port_toggle_bits = gpio_tc4x_port_toggle_bits,
	.pin_interrupt_configure = gpio_tc4x_pin_interrupt_configure,
	.manage_callback = gpio_tc4x_manage_callback,
};

/**
 * @brief Initialize GPIO port
 *
 * Perform basic initialization of a GPIO port. The code will
 * enable the clock for corresponding peripheral.
 *
 * @param dev GPIO device struct
 *
 * @return 0
 */
static int gpio_tc4x_init(const struct device *dev)
{
	const struct gpio_tc4x_config *cfg = dev->config;
	struct gpio_tc4x_data *data = dev->data;

	cfg->config_func(dev);
	sys_slist_init(&data->callbacks);

	return 0;
}

#define GPIO_TC4X_ISR(inst)                                                                       \
	static void __maybe_unused gpio_tc4x_isr_##inst(struct gpio_tc4x_irq_source *irq_src)     \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(inst);                               \
		struct gpio_tc4x_data *data = dev->data;                                          \
		if (irq_src->type == TC4X_IRQ_TYPE_ERU) {                                         \
			if (MODULE_SCU.ERU.EICR[irq_src->ch].B.LDEN) {                             \
				if ((MODULE_SCU.ERU.EIFR.U & BIT(irq_src->ch)) != 0) {             \
					/* Retrigger ERU IRQ for level type irqs */                \
					MODULE_SCU.ERU.FMR.U = BIT(irq_src->ch);                   \
				}                                                                  \
			} else {                                                                   \
				MODULE_SCU.ERU.FMR.U = BIT(irq_src->ch + 16);                      \
			}                                                                          \
		}                                                                                  \
		gpio_fire_callbacks(&data->callbacks, dev, BIT(irq_src->pin));                     \
	}

#define GPIO_TC4X_IRQ_CONFIGURE(n, inst)                                                          \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    gpio_tc4x_isr_##inst, &gpio_tc4x_irq_sources_##inst[n], 0);                   \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define GPIO_TC4X_ALL_IRQS(inst) LISTIFY(DT_INST_NUM_IRQS(inst), GPIO_TC4X_IRQ_CONFIGURE, (), inst)

#define GPIO_TC4X_CONFIG_FUNC(n)                                                                  \
	GPIO_TC4X_ISR(n)                                                                          \
	static void gpio_tc4x_config_func_##n(const struct device *dev)                           \
	{                                                                                          \
		GPIO_TC4X_ALL_IRQS(n)                                                             \
	}

#define GPIO_TC4X_IRQ_SOURCE(node_id, prop, i)                                                    \
	{(DT_PROP_BY_IDX(node_id, prop, i) & 0xF),                                                 \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 4) & 0xF),                                          \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 8) & 0xF),                                          \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 12) & 0x3),                                         \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 28) & 0xF)}
#define GPIO_TC4X_IRQ_SOURCE_IS_GTM(node_id, prop, id)                                            \
	(((DT_PROP_BY_IDX(node_id, prop, id) >> 12) & 0x3) == TC4X_IRQ_TYPE_GTM)
#define GPIO_TC4X_IRQ_SOURCE_IS_ERU(node_id, prop, id)                                            \
	(((DT_PROP_BY_IDX(node_id, prop, id) >> 12) & 0x3) == TC4X_IRQ_TYPE_ERU)
#define GPIO_TC4X_ENABLE_GTM(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, irq_sources),		\
		(DT_INST_FOREACH_PROP_ELEM_SEP(n, irq_sources,		\
			GPIO_TC4X_IRQ_SOURCE_IS_GTM, (||))),		\
		(false))
#define GPIO_TC4X_ENABLE_ERU(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, irq_sources),		\
		(DT_INST_FOREACH_PROP_ELEM_SEP(n, irq_sources,		\
			GPIO_TC4X_IRQ_SOURCE_IS_ERU, (||))),		\
		(false))
#define GPIO_TC4X_INIT(n)                                                                         \
	static const struct gpio_tc4x_irq_source gpio_tc4x_irq_sources_##n[] = {COND_CODE_1(     \
	DT_INST_NODE_HAS_PROP(n, irq_sources),                                             \
	(DT_INST_FOREACH_PROP_ELEM_SEP(n, irq_sources, GPIO_TC4X_IRQ_SOURCE, (,))),      \
	())};      \
	GPIO_TC4X_CONFIG_FUNC(n)                                                                  \
	static const struct gpio_tc4x_config gpio_tc4x_config_##n = {                            \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = (Ifx_P *)DT_INST_REG_ADDR(n),                                              \
		.irq_source_count = DT_INST_PROP_LEN_OR(n, irq_sources, 0),                        \
		.irq_sources = gpio_tc4x_irq_sources_##n,                                         \
		.config_func = gpio_tc4x_config_func_##n,                                         \
		.enable_gtm = GPIO_TC4X_ENABLE_GTM(n),                                            \
	};                                                                                         \
                                                                                                   \
	static struct gpio_tc4x_data gpio_tc4x_data_##n = {};                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_tc4x_init, NULL, &gpio_tc4x_data_##n,                      \
			      &gpio_tc4x_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,     \
			      &tc4x_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TC4X_INIT)
