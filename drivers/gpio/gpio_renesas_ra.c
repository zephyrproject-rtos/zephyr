/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_gpio

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/intc_ra_icu.h>
#include <zephyr/drivers/pinctrl.h>

enum {
	PCNTR1_OFFSET = 0x0,
	PCNTR2_OFFSET = 0x4,
	PCNTR3_OFFSET = 0x8,
	PCNTR4_OFFSET = 0xc
};

enum {
	PCNTR1_PDR0_OFFSET = 0,
	PCNTR1_PODR0_OFFSET = 16,
};

enum {
	PCNTR2_PIDR0_OFFSET = 0,
	PCNTR2_EIDR0_OFFSET = 16,
};

enum {
	PCNTR3_POSR0_OFFSET = 0,
	PCNTR3_PORR0_OFFSET = 16,
};

enum {
	PCNTR4_EOSR0_OFFSET = 0,
	PCNTR4_EORR0_OFFSET = 16,
};

struct gpio_ra_irq_info {
	const uint8_t *const pins;
	size_t num;
	int port_irq;
	int irq;
	uint32_t priority;
	uint32_t flags;
	ra_isr_handler isr;
};

struct gpio_ra_pin_irq_info {
	const struct gpio_ra_irq_info *info;
	uint8_t pin;
};

struct gpio_ra_config {
	struct gpio_driver_config common;
	mem_addr_t regs;
	struct gpio_ra_irq_info *irq_info;
	uint32_t irq_info_size;
	uint16_t port;
};

struct gpio_ra_data {
	struct gpio_driver_data common;
	struct gpio_ra_pin_irq_info port_irq_info[16];
	sys_slist_t callbacks;
};

static inline uint32_t gpio_ra_irq_info_event(const struct gpio_ra_irq_info *info)
{
	return ((info->flags & RA_ICU_FLAG_EVENT_MASK) >> RA_ICU_FLAG_EVENT_OFFSET);
}

static void gpio_ra_isr(const struct device *dev, uint32_t port_irq)
{
	struct gpio_ra_data *data = dev->data;
	const struct gpio_ra_pin_irq_info *pin_irq = &data->port_irq_info[port_irq];
	const int irq = ra_icu_query_exists_irq(gpio_ra_irq_info_event(pin_irq->info));

	if (irq >= 0) {
		gpio_fire_callbacks(&data->callbacks, dev, BIT(pin_irq->pin));
		ra_icu_clear_int_flag(irq);
	}
}

static const struct gpio_ra_irq_info *query_irq_info(const struct device *dev, uint32_t pin)
{
	const struct gpio_ra_config *config = dev->config;

	for (int i = 0; i < config->irq_info_size; i++) {
		const struct gpio_ra_irq_info *info = &config->irq_info[i];

		for (int j = 0; j < info->num; j++) {
			if (info->pins[j] == pin) {
				return info;
			}
		}
	}

	return NULL;
}

static inline uint32_t reg_read(const struct device *dev, size_t offset)
{
	const struct gpio_ra_config *config = dev->config;

	return sys_read32(config->regs + offset);
}

static inline void reg_write(const struct device *dev, size_t offset, uint32_t value)
{
	const struct gpio_ra_config *config = dev->config;

	sys_write32(value, config->regs + offset);
}

static inline uint32_t port_read(const struct device *dev)
{
	return reg_read(dev, PCNTR2_OFFSET) & UINT16_MAX;
}

static int port_write(const struct device *dev, uint16_t value, uint16_t mask)
{
	const uint16_t set = value & mask;
	const uint16_t clr = (~value) & mask;

	reg_write(dev, PCNTR3_OFFSET, (clr << PCNTR3_PORR0_OFFSET) | set << PCNTR3_POSR0_OFFSET);

	return 0;
}

static int gpio_ra_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const enum gpio_int_mode mode =
		flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE);
	const enum gpio_int_trig trig = flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1);
	const struct gpio_ra_config *config = dev->config;
	struct gpio_ra_data *data = dev->data;
	struct ra_pinctrl_soc_pin pincfg = {0};

	if ((flags & GPIO_OUTPUT) && (flags & GPIO_INPUT)) {
		/* Pin cannot be configured as input and output */
		return -ENOTSUP;
	} else if (!(flags & (GPIO_INPUT | GPIO_OUTPUT))) {
		/* Pin has to be configured as input or output */
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		pincfg.cfg |= BIT(R_PFS_PORT_PIN_PmnPFS_PDR_Pos);
	}

	if (flags & GPIO_PULL_UP) {
		pincfg.cfg |= BIT(R_PFS_PORT_PIN_PmnPFS_PCR_Pos);
	}

	if ((flags & GPIO_SINGLE_ENDED) && (flags & GPIO_LINE_OPEN_DRAIN)) {
		pincfg.cfg |= BIT(R_PFS_PORT_PIN_PmnPFS_NCODR_Pos);
	}

	if (flags & GPIO_INT_ENABLE) {
		pincfg.cfg |= BIT(R_PFS_PORT_PIN_PmnPFS_ISEL_Pos);
	}

	pincfg.cfg &= ~BIT(R_PFS_PORT_PIN_PmnPFS_PMR_Pos);

	pincfg.pin_num = pin;
	pincfg.port_num = config->port;

	if (flags & GPIO_INT_ENABLE) {
		const struct gpio_ra_irq_info *irq_info;
		uint32_t intcfg;
		int irqn;

		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig != GPIO_INT_TRIG_LOW) {
				return -ENOTSUP;
			}

			intcfg = ICU_LOW_LEVEL;
		} else if (mode == GPIO_INT_MODE_EDGE) {
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				intcfg = ICU_FALLING;
				break;
			case GPIO_INT_TRIG_HIGH:
				intcfg = ICU_RISING;
				break;
			case GPIO_INT_TRIG_BOTH:
				intcfg = ICU_BOTH_EDGE;
				break;
			default:
				return -ENOTSUP;
			}
		} else {
			return -ENOTSUP;
		}

		irq_info = query_irq_info(dev, pin);
		if (irq_info == NULL) {
			return -EINVAL;
		}

		irqn = ra_icu_irq_connect_dynamic(
			irq_info->irq, irq_info->priority, irq_info->isr, dev,
			(intcfg << RA_ICU_FLAG_INTCFG_OFFSET) | irq_info->flags);
		if (irqn < 0) {
			return irqn;
		}

		data->port_irq_info[irq_info->port_irq].pin = pin;
		data->port_irq_info[irq_info->port_irq].info = irq_info;

		irq_enable(irqn);
	}

	return pinctrl_configure_pins(&pincfg, 1, PINCTRL_REG_NONE);
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_ra_pin_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_ra_config *config = dev->config;
	const struct gpio_ra_irq_info *irq_info;
	struct ra_pinctrl_soc_pin pincfg;
	ra_isr_handler cb;
	const void *cbarg;
	uint32_t intcfg;
	int irqn;
	int err;

	memset(flags, 0, sizeof(gpio_flags_t));

	err = ra_pinctrl_query_config(config->port, pin, &pincfg);
	if (err < 0) {
		return err;
	}

	if (pincfg.cfg & BIT(R_PFS_PORT_PIN_PmnPFS_PDR_Pos)) {
		*flags |= GPIO_OUTPUT;
	} else {
		*flags |= GPIO_INPUT;
	}

	if (pincfg.cfg & BIT(R_PFS_PORT_PIN_PmnPFS_ISEL_Pos)) {
		*flags |= GPIO_INT_ENABLE;
	}

	if (pincfg.cfg & BIT(R_PFS_PORT_PIN_PmnPFS_PCR_Pos)) {
		*flags |= GPIO_PULL_UP;
	}

	irq_info = query_irq_info(dev, pin);
	if (irq_info == NULL) {
		return 0;
	}

	irqn = ra_icu_query_exists_irq(gpio_ra_irq_info_event(irq_info));
	if (irqn < 0) {
		return 0;
	}

	ra_icu_query_irq_config(irqn, &intcfg, &cb, &cbarg);

	if (cbarg != dev) {
		return 0;
	}

	if (intcfg == ICU_FALLING) {
		*flags |= GPIO_INT_TRIG_LOW;
		*flags |= GPIO_INT_MODE_EDGE;
	} else if (intcfg == ICU_RISING) {
		*flags |= GPIO_INT_TRIG_HIGH;
		*flags |= GPIO_INT_MODE_EDGE;
	} else if (intcfg == ICU_BOTH_EDGE) {
		*flags |= GPIO_INT_TRIG_BOTH;
		*flags |= GPIO_INT_MODE_EDGE;
	} else if (intcfg == ICU_LOW_LEVEL) {
		*flags |= GPIO_INT_TRIG_LOW;
		*flags |= GPIO_INT_MODE_LEVEL;
	}

	return 0;
}
#endif

static int gpio_ra_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	*value = port_read(dev);

	return 0;
}

static int gpio_ra_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	uint16_t port_val;

	port_val = port_read(dev);
	port_val = (port_val & ~mask) | (value & mask);
	return port_write(dev, port_val, UINT16_MAX);
}

static int gpio_ra_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	uint16_t port_val;

	port_val = port_read(dev);
	port_val |= pins;
	return port_write(dev, port_val, UINT16_MAX);
}

static int gpio_ra_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	uint16_t port_val;

	port_val = port_read(dev);
	port_val &= ~pins;
	return port_write(dev, port_val, UINT16_MAX);
}

static int gpio_ra_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	uint16_t port_val;

	port_val = port_read(dev);
	port_val ^= pins;
	return port_write(dev, port_val, UINT16_MAX);
}

static int gpio_ra_manage_callback(const struct device *dev, struct gpio_callback *callback,
				   bool set)
{
	struct gpio_ra_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_ra_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	gpio_flags_t pincfg;
	int err;

	err = gpio_ra_pin_get_config(dev, pin, &pincfg);
	if (err < 0) {
		return err;
	}

	return gpio_ra_pin_configure(dev, pin, pincfg | mode | trig);
}

static DEVICE_API(gpio, gpio_ra_driver_api) = {
	.pin_configure = gpio_ra_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_ra_pin_get_config,
#endif
	.port_get_raw = gpio_ra_port_get_raw,
	.port_set_masked_raw = gpio_ra_port_set_masked_raw,
	.port_set_bits_raw = gpio_ra_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ra_port_clear_bits_raw,
	.port_toggle_bits = gpio_ra_port_toggle_bits,
	.pin_interrupt_configure = gpio_ra_pin_interrupt_configure,
	.manage_callback = gpio_ra_manage_callback,
};

#define RA_NUM_PORT_IRQ0  0
#define RA_NUM_PORT_IRQ1  1
#define RA_NUM_PORT_IRQ2  2
#define RA_NUM_PORT_IRQ3  3
#define RA_NUM_PORT_IRQ4  4
#define RA_NUM_PORT_IRQ5  5
#define RA_NUM_PORT_IRQ6  6
#define RA_NUM_PORT_IRQ7  7
#define RA_NUM_PORT_IRQ8  8
#define RA_NUM_PORT_IRQ9  9
#define RA_NUM_PORT_IRQ10 10
#define RA_NUM_PORT_IRQ11 11
#define RA_NUM_PORT_IRQ12 12
#define RA_NUM_PORT_IRQ13 13
#define RA_NUM_PORT_IRQ14 14
#define RA_NUM_PORT_IRQ15 15

#define GPIO_RA_DECL_PINS(n, p, i)                                                                 \
	const uint8_t _CONCAT(n, ___pins##i[]) = {DT_FOREACH_PROP_ELEM_SEP(                        \
		n, _CONCAT(DT_STRING_TOKEN_BY_IDX(n, p, i), _pins), DT_PROP_BY_IDX, (,))};

#define GPIO_RA_IRQ_INFO(n, p, i)                                                                  \
	{                                                                                          \
		.port_irq = _CONCAT(RA_NUM_, DT_STRING_UPPER_TOKEN_BY_IDX(n, p, i)),               \
		.irq = DT_IRQ_BY_IDX(n, i, irq),                                                   \
		.flags = DT_IRQ_BY_IDX(n, i, flags),                                               \
		.priority = DT_IRQ_BY_IDX(n, i, priority),                                         \
		.pins = _CONCAT(n, ___pins##i),                                                    \
		.num = ARRAY_SIZE(_CONCAT(n, ___pins##i)),                                         \
		.isr = _CONCAT(n, _CONCAT(gpio_ra_isr_, DT_STRING_TOKEN_BY_IDX(n, p, i))),         \
	},

#define GPIO_RA_ISR_DECL(n, p, i)                                                                  \
	static void _CONCAT(n, _CONCAT(gpio_ra_isr_, DT_STRING_TOKEN_BY_IDX(n, p, i)))(            \
		const void *arg)                                                                   \
	{                                                                                          \
		gpio_ra_isr((const struct device *)arg,                                            \
			    _CONCAT(RA_NUM_, DT_STRING_UPPER_TOKEN_BY_IDX(n, p, i)));              \
	}

#define GPIO_RA_INIT(idx)                                                                          \
	static struct gpio_ra_data gpio_ra_data_##idx = {};                                        \
	DT_INST_FOREACH_PROP_ELEM(idx, interrupt_names, GPIO_RA_DECL_PINS);                        \
	DT_INST_FOREACH_PROP_ELEM(idx, interrupt_names, GPIO_RA_ISR_DECL);                         \
	struct gpio_ra_irq_info gpio_ra_irq_info_##idx[] = {                                       \
		DT_INST_FOREACH_PROP_ELEM(idx, interrupt_names, GPIO_RA_IRQ_INFO)};                \
	static struct gpio_ra_config gpio_ra_config_##idx = {                                      \
		.common = {                                                                        \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),                     \
		},                                                                                 \
		.regs = DT_INST_REG_ADDR(idx),                                                     \
		.port = (DT_INST_REG_ADDR(idx) - DT_REG_ADDR(DT_NODELABEL(ioport0))) /             \
			DT_INST_REG_SIZE(idx),                                                     \
		.irq_info = gpio_ra_irq_info_##idx,                                                \
		.irq_info_size = ARRAY_SIZE(gpio_ra_irq_info_##idx),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, NULL, NULL, &gpio_ra_data_##idx, &gpio_ra_config_##idx,         \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RA_INIT)
