/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tc3x_gpio

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <errno.h>

#include "IfxGtm_reg.h"
#include "IfxScu_reg.h"
#include "soc.h"

#include "gpio_tc3x.h"

/* TC3x GTM TIM unit base, with 0x800 stride per unit and 0x80 stride per
 * channel. The Ifx_GTM_TIM struct has named CHx fields with reserved gaps,
 * so step through raw addresses instead of array indexing.
 */
#define TC3X_GTM_TIM_BASE            0xF0101000u
#define TC3X_GTM_TIM_STRIDE          0x800u
#define TC3X_GTM_TIM_CH_STRIDE       0x80u
#define TC3X_GTM_TIM_CH_CTRL_OFF     0x24
#define TC3X_GTM_TIM_CH_NOTIFY_OFF   0x2C
#define TC3X_GTM_TIM_CH_IRQ_EN_OFF   0x30
#define TC3X_GTM_TIM_CH_IRQ_MODE_OFF 0x38

static int gpio_tc3x_flags_to_iocr(gpio_flags_t flags, uint32_t *iocr)
{
	bool is_input = flags & GPIO_INPUT;
	bool is_output = flags & GPIO_OUTPUT;

	if (!is_input && !is_output) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	if (is_output && (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if (is_input) {
		if (flags & (GPIO_PULL_UP)) {
			*iocr = TC3X_GPIO_MODE_INPUT_PULL_UP;
		} else if (flags & (GPIO_PULL_DOWN)) {
			*iocr = TC3X_GPIO_MODE_INPUT_PULL_DOWN;
		} else {
			*iocr = TC3X_GPIO_MODE_INPUT_TRISTATE;
		}
	}
	if (is_output) {
		if (flags & GPIO_OPEN_DRAIN) {
			*iocr = TC3X_GPIO_MODE_OUTPUT_OPEN_DRAIN;
		} else {
			*iocr = TC3X_GPIO_MODE_OUTPUT_PUSH_PULL;
		}
	}

	return 0;
}

#if defined(CONFIG_GPIO_GET_CONFIG)
static int gpio_tc3x_pincfg_to_flags(uint32_t iocr, uint32_t out, gpio_flags_t *out_flags)
{
	if (iocr & TC3X_IOCR_OUTPUT) {
		if (out) {
			*out_flags = GPIO_OUTPUT_HIGH;
		} else {
			*out_flags = GPIO_OUTPUT_LOW;
		}
		if (iocr & TC3X_IOCR_OPEN_DRAIN) {
			*out_flags |= GPIO_OPEN_DRAIN;
		}
	} else {
		*out_flags = GPIO_INPUT;
		if (iocr & TC3X_IOCR_PULL_DOWN) {
			*out_flags |= GPIO_PULL_DOWN;
		} else if (iocr & TC3X_IOCR_PULL_UP) {
			*out_flags |= GPIO_PULL_UP;
		}
	}

	return 0;
}
#endif

static int gpio_tc3x_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	*value = sys_read32(cfg->base + TC3X_IN_OFFSET);

	return 0;
}

static int gpio_tc3x_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	uint32_t clear, set;

	mask &= 0xFFFFU;
	value &= 0xFFFFU;

	set = (mask & value);
	clear = (mask & ~value);

	sys_write32((clear << 16) | set, cfg->base + TC3X_OMR_OFFSET);

	return 0;
}

static int gpio_tc3x_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	sys_write32(0xFFFFU & pins, cfg->base + TC3X_OMR_OFFSET);

	return 0;
}

static int gpio_tc3x_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	sys_write32((0xFFFFU & pins) << 16, cfg->base + TC3X_OMR_OFFSET);

	return 0;
}

static int gpio_tc3x_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	uint32_t out;
	uint64_t swap;

	do {
		out = sys_read32(cfg->base + TC3X_OUT_OFFSET);
		swap = ((uint64_t)out << 32) | (out ^ pins);
		__asm("\tcmpswap.w [%1]+0, %A0\n" : "+d"(swap) : "a"((void *)cfg->base));
	} while ((swap & 0xFFFFFFFFU) != out);

	return 0;
}

static int gpio_tc3x_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	int err;
	uint32_t iocr = 0;

	err = gpio_tc3x_flags_to_iocr(flags, &iocr);
	if (err != 0) {
		return err;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_tc3x_port_set_bits_raw(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_tc3x_port_clear_bits_raw(dev, BIT(pin));
		}
	}

	__asm("\timask %%e14, %0, %1, 5\n"
	      "\tldmst [%2]+0, %%e14\n"
	      :
	      : "d"(iocr), "d"((pin & 0x3) * 8 + 3),
		"a"(cfg->base + TC3X_IOCR_OFFSET + (pin >> 2) * 4)
	      : "e14");

	return 0;
}

#if defined(CONFIG_GPIO_GET_CONFIG)
static int gpio_tc3x_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_tc3x_config *cfg = dev->config;

	gpio_tc3x_pincfg_to_flags(sys_read32(cfg->base + TC3X_IOCR_OFFSET),
				  sys_read32(cfg->base + TC3X_OUT_OFFSET), flags);

	return 0;
}
#endif

static int gpio_tc3x_gtm_init_once(void)
{
	static bool initialized;
	Ifx_GTM_CMU_CLK_EN clk_en;

	if (initialized) {
		return 0;
	}

	if (MODULE_GTM.CLC.B.DISS == 1) {
		if (!aurix_enable_clock((uintptr_t)&MODULE_GTM.CLC, 1000)) {
			return -EIO;
		}
	}

	/* GCLK_NUM/DEN are CPU-EndInit protected on TC3x. */
	aurix_cpu_endinit_enable(false);
	sys_write32(1, (mem_addr_t)&MODULE_GTM.CMU.GCLK_NUM);
	sys_write32(1, (mem_addr_t)&MODULE_GTM.CMU.GCLK_DEN);
	aurix_cpu_endinit_enable(true);

	clk_en.U = 0;
	clk_en.B.EN_CLK0 = 0x2;
	clk_en.B.EN_CLK1 = 0x2;
	clk_en.B.EN_CLK2 = 0x2;
	clk_en.B.EN_CLK3 = 0x2;
	clk_en.B.EN_CLK4 = 0x2;
	clk_en.B.EN_CLK5 = 0x2;
	clk_en.B.EN_CLK6 = 0x2;
	clk_en.B.EN_CLK7 = 0x2;
	clk_en.B.EN_ECLK0 = 0x2;
	clk_en.B.EN_ECLK1 = 0x2;
	clk_en.B.EN_ECLK2 = 0x2;
	clk_en.B.EN_FXCLK = 0x2;
	sys_write32(clk_en.U, (mem_addr_t)&MODULE_GTM.CMU.CLK_EN);

	initialized = true;
	return 0;
}

static int gpio_tc3x_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	const struct gpio_tc3x_irq_source *irq_src = NULL;

	for (uint32_t i = 0; i < cfg->irq_source_count; i++) {
		if (cfg->irq_sources[i].pin == pin) {
			irq_src = &cfg->irq_sources[i];
			break;
		}
	}

	if (irq_src == NULL) {
		return -ENOTSUP;
	}

	if (irq_src->type == TC3X_IRQ_TYPE_GTM) {
		struct gpio_tc3x_data *data = dev->data;
		uintptr_t ch_base;
		Ifx_GTM_TIM_CH_CTRL ctrl;
		int ret;

		ret = gpio_tc3x_gtm_init_once();
		if (ret) {
			return ret;
		}

		MODULE_GTM.TIMINSEL[irq_src->cls].U =
			(MODULE_GTM.TIMINSEL[irq_src->cls].U & ~(0xFu << (4 * irq_src->ch))) |
			((uint32_t)irq_src->mux << (4 * irq_src->ch));

		ch_base = TC3X_GTM_TIM_BASE +
			  (uintptr_t)irq_src->cls * TC3X_GTM_TIM_STRIDE +
			  (uintptr_t)irq_src->ch * TC3X_GTM_TIM_CH_STRIDE;

		/* TIM_MODE 2 (TPIM) fires NEWVAL on each selected edge. For
		 * level interrupts we configure both edges (ISL=1) and emulate
		 * level semantics by retrying the callback in the ISR while
		 * the pin holds the requested level. DSL is also passed
		 * through so the ISR can compare against the live pin value.
		 */
		ctrl.U = 0;
		ctrl.B.CLK_SEL = 7;
		ctrl.B.TIM_MODE = 0x2;
		if (mode == GPIO_INT_MODE_LEVEL) {
			ctrl.B.DSL = (trig & GPIO_INT_TRIG_HIGH) != 0 ? 1 : 0;
			ctrl.B.ISL = 1;
			data->level_pins |= BIT(pin);
		} else {
			ctrl.B.DSL = trig == GPIO_INT_TRIG_HIGH ? 1 : 0;
			ctrl.B.ISL = trig == GPIO_INT_TRIG_BOTH ? 1 : 0;
			data->level_pins &= ~BIT(pin);
		}

		sys_write32(ctrl.U, ch_base + TC3X_GTM_TIM_CH_CTRL_OFF);
		sys_write32(0x3F, ch_base + TC3X_GTM_TIM_CH_NOTIFY_OFF);
		sys_write32(0x1, ch_base + TC3X_GTM_TIM_CH_IRQ_MODE_OFF);
		sys_write32(mode != GPIO_INT_MODE_DISABLED ? 0x1 : 0x0,
			    ch_base + TC3X_GTM_TIM_CH_IRQ_EN_OFF);

		if (mode != GPIO_INT_MODE_DISABLED) {
			ctrl.B.TIM_EN = 1;
			sys_write32(ctrl.U, ch_base + TC3X_GTM_TIM_CH_CTRL_OFF);
		}

		return 0;
	}

	if (irq_src->type == TC3X_IRQ_TYPE_ERU) {
		Ifx_SCU_EICR eicr = MODULE_SCU.EICR[irq_src->ch / 2];
		Ifx_SCU_IGCR igcr = MODULE_SCU.IGCR[irq_src->cls / 2];

		if ((irq_src->ch & 1) == 0) {
			eicr.B.EIEN0 = mode != GPIO_INT_MODE_DISABLED;
			eicr.B.EXIS0 = irq_src->mux;
			eicr.B.INP0 = irq_src->cls;
			eicr.B.REN0 = (trig & GPIO_INT_TRIG_HIGH) != 0;
			eicr.B.FEN0 = (trig & GPIO_INT_TRIG_LOW) != 0;
			eicr.B.LDEN0 = (mode == GPIO_INT_MODE_LEVEL);
		} else {
			eicr.B.EIEN1 = mode != GPIO_INT_MODE_DISABLED;
			eicr.B.EXIS1 = irq_src->mux;
			eicr.B.INP1 = irq_src->cls;
			eicr.B.REN1 = (trig & GPIO_INT_TRIG_HIGH) != 0;
			eicr.B.FEN1 = (trig & GPIO_INT_TRIG_LOW) != 0;
			eicr.B.LDEN1 = (mode == GPIO_INT_MODE_LEVEL);
		}
		MODULE_SCU.EICR[irq_src->ch / 2] = eicr;

		/* The IGCR holds two OGUs per register (sub 0..1). Each OGU has
		 * its own GEEN/IGP and an IPEN bitmap of input channels. Mirror
		 * the TC4x ERU branch: set IPEN[ch] for level mode, IGP=1 for
		 * edge / 2 for level so the gating logic emits a trigger.
		 */
		if ((irq_src->cls & 1) == 0) {
			igcr.U &= ~0x0000C0FFu;
			if (mode == GPIO_INT_MODE_LEVEL) {
				igcr.U |= BIT(irq_src->ch);
			}
			igcr.B.GEEN0 = (mode != GPIO_INT_MODE_DISABLED);
			igcr.B.IGP0 = mode == GPIO_INT_MODE_EDGE ? 1 : 2;
		} else {
			igcr.U &= ~0xC0FF0000u;
			if (mode == GPIO_INT_MODE_LEVEL) {
				igcr.U |= ((uint32_t)BIT(irq_src->ch)) << 16;
			}
			igcr.B.GEEN1 = (mode != GPIO_INT_MODE_DISABLED);
			igcr.B.IGP1 = mode == GPIO_INT_MODE_EDGE ? 1 : 2;
		}
		MODULE_SCU.IGCR[irq_src->cls / 2] = igcr;

		return 0;
	}

	return -ENOTSUP;
}

static int gpio_tc3x_manage_callback(const struct device *dev, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_tc3x_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static DEVICE_API(gpio, gpio_tc3x_driver) = {
	.pin_configure = gpio_tc3x_config,
#if defined(CONFIG_GPIO_GET_CONFIG)
	.pin_get_config = gpio_tc3x_get_config,
#endif /* CONFIG_GPIO_GET_CONFIG */
	.port_get_raw = gpio_tc3x_port_get_raw,
	.port_set_masked_raw = gpio_tc3x_port_set_masked_raw,
	.port_set_bits_raw = gpio_tc3x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tc3x_port_clear_bits_raw,
	.port_toggle_bits = gpio_tc3x_port_toggle_bits,
	.pin_interrupt_configure = gpio_tc3x_pin_interrupt_configure,
	.manage_callback = gpio_tc3x_manage_callback,
};

static int gpio_tc3x_init(const struct device *dev)
{
	const struct gpio_tc3x_config *cfg = dev->config;
	struct gpio_tc3x_data *data = dev->data;

	sys_slist_init(&data->callbacks);
	if (cfg->config_func) {
		cfg->config_func(dev);
	}

	return 0;
}

#define GPIO_TC3X_ISR(inst)                                                                        \
	static void __maybe_unused gpio_tc3x_isr_##inst(struct gpio_tc3x_irq_source *irq_src)      \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(inst);                               \
		const struct gpio_tc3x_config *cfg = dev->config;                                  \
		struct gpio_tc3x_data *data = dev->data;                                           \
		if (irq_src->type == TC3X_IRQ_TYPE_GTM) {                                          \
			uintptr_t ch_base = TC3X_GTM_TIM_BASE +                                    \
					    (uintptr_t)irq_src->cls * TC3X_GTM_TIM_STRIDE +        \
					    (uintptr_t)irq_src->ch * TC3X_GTM_TIM_CH_STRIDE;       \
			sys_write32(0x3F, ch_base + TC3X_GTM_TIM_CH_NOTIFY_OFF);                   \
			gpio_fire_callbacks(&data->callbacks, dev, BIT(irq_src->pin));             \
			if ((data->level_pins & BIT(irq_src->pin)) != 0) {                         \
				Ifx_GTM_TIM_CH_CTRL ctrl;                                          \
				uint32_t in;                                                       \
				ctrl.U = sys_read32(ch_base + TC3X_GTM_TIM_CH_CTRL_OFF);           \
				in = sys_read32(cfg->base + TC3X_IN_OFFSET);                       \
				while ((sys_read32(ch_base + TC3X_GTM_TIM_CH_IRQ_EN_OFF) & 0x1) && \
				       (((in >> irq_src->pin) & 1) == ctrl.B.DSL)) {               \
					gpio_fire_callbacks(&data->callbacks, dev,                 \
							    BIT(irq_src->pin));                    \
					in = sys_read32(cfg->base + TC3X_IN_OFFSET);               \
				}                                                                  \
			}                                                                          \
			return;                                                                    \
		}                                                                                  \
		if (irq_src->type == TC3X_IRQ_TYPE_ERU) {                                          \
			uint32_t lden = ((irq_src->ch & 1) == 0)                                   \
						? MODULE_SCU.EICR[irq_src->ch / 2].B.LDEN0         \
						: MODULE_SCU.EICR[irq_src->ch / 2].B.LDEN1;        \
			if (lden) {                                                                \
				if ((MODULE_SCU.EIFR.U & BIT(irq_src->ch)) != 0) {                 \
					MODULE_SCU.FMR.U = BIT(irq_src->ch);                       \
				}                                                                  \
			} else {                                                                   \
				MODULE_SCU.FMR.U = BIT(irq_src->ch + 16);                          \
			}                                                                          \
		}                                                                                  \
		gpio_fire_callbacks(&data->callbacks, dev, BIT(irq_src->pin));                     \
	}

#define GPIO_TC3X_IRQ_CONFIGURE(n, inst)                                                           \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    gpio_tc3x_isr_##inst, &gpio_tc3x_irq_sources_##inst[n], 0);                    \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define GPIO_TC3X_ALL_IRQS(inst) LISTIFY(DT_INST_NUM_IRQS(inst), GPIO_TC3X_IRQ_CONFIGURE, (), inst)

#define GPIO_TC3X_CONFIG_FUNC(n)                                                                   \
	GPIO_TC3X_ISR(n)                                                                           \
	static void gpio_tc3x_config_func_##n(const struct device *dev)                            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		GPIO_TC3X_ALL_IRQS(n)                                                              \
	}

#define GPIO_TC3X_IRQ_SOURCE(node_id, prop, i)                                                     \
	{(DT_PROP_BY_IDX(node_id, prop, i) & 0xF),                                                 \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 4) & 0xF),                                          \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 8) & 0xF),                                          \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 12) & 0x3),                                         \
	 ((DT_PROP_BY_IDX(node_id, prop, i) >> 28) & 0xF)}

#define GPIO_TC3X_INIT(n)                                                                          \
	static const struct gpio_tc3x_irq_source gpio_tc3x_irq_sources_##n[] = {COND_CODE_1(       \
		DT_INST_NODE_HAS_PROP(n, irq_sources),                                             \
		(DT_INST_FOREACH_PROP_ELEM_SEP(n, irq_sources, GPIO_TC3X_IRQ_SOURCE, (,))),        \
		())};                                                                              \
	GPIO_TC3X_CONFIG_FUNC(n)                                                                   \
	static const struct gpio_tc3x_config gpio_tc3x_config_##n = {                              \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.config_func = gpio_tc3x_config_func_##n,                                          \
		.irq_sources = gpio_tc3x_irq_sources_##n,                                          \
		.irq_source_count = DT_INST_PROP_LEN_OR(n, irq_sources, 0),                        \
	};                                                                                         \
                                                                                                   \
	static struct gpio_tc3x_data gpio_tc3x_data_##n = {};                                      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_tc3x_init, NULL, &gpio_tc3x_data_##n, &gpio_tc3x_config_##n, \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_tc3x_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TC3X_INIT)
