/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_gpio

#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/intc_rx_icu.h>
#include <zephyr/drivers/misc/renesas_rx_external_interrupt/renesas_rx_external_interrupt.h>
#include <zephyr/irq.h>
#include <soc.h>
#include "r_mpc_rx_if.h"

struct gpio_rx_irq_info {
	const struct device *port_irq;
	const uint8_t *const pins;
	size_t num;
};

struct gpio_rx_config {
	struct gpio_driver_config common;
	uint8_t port_num;
	volatile uint8_t *pinmux;
	volatile struct {
		volatile uint8_t *pdr;
		volatile uint8_t *podr;
		volatile uint8_t *pidr;
		volatile uint8_t *pmr;
		volatile uint8_t *odr0;
		volatile uint8_t *odr1;
		volatile uint8_t *pcr;
		volatile uint8_t *dscr;
		volatile uint8_t *dscr2;
	} reg;
#ifdef CONFIG_RENESAS_RX_EXTERNAL_INTERRUPT
	const struct gpio_rx_irq_info *irq_info;
	const size_t irq_info_size;
#endif
};

struct gpio_rx_data {
	struct gpio_driver_data common;
#ifdef CONFIG_RENESAS_RX_EXTERNAL_INTERRUPT
	sys_slist_t callbacks;
	struct k_spinlock lock;
#endif
};

#ifdef CONFIG_RENESAS_RX_EXTERNAL_INTERRUPT
static const struct gpio_rx_irq_info *query_irq_info(const struct device *dev, uint32_t pin)
{
	const struct gpio_rx_config *config = dev->config;

	for (int i = 0; i < config->irq_info_size; i++) {
		const struct gpio_rx_irq_info *info = &config->irq_info[i];

		for (int j = 0; j < info->num; j++) {
			if (info->pins[j] == pin) {
				return info;
			}
		}
	}

	return NULL;
}

static void gpio_rx_callback_adapter(const struct device *dev, gpio_pin_t pin)
{
	struct gpio_rx_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
}

void mpc_set_irq(const struct device *dev, gpio_pin_t pin, bool set)
{
	const struct gpio_rx_config *cfg = dev->config;

	/* Enable writing to MPC registers. */
	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);

	WRITE_BIT(*(cfg->pinmux + pin), PFS_BIT_ISEL, set);

	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);
}

#endif

static int gpio_rx_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rx_config *cfg = dev->config;
	struct rx_pinctrl_soc_pin pincfg;

	memset(&pincfg, 0, sizeof(pincfg));

	if (((flags & GPIO_INPUT) != 0U) && ((flags & GPIO_OUTPUT) != 0U)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_DOWN) != 0U) {
		return -ENOTSUP;
	}

	if (!IS_ENABLED(CONFIG_RENESAS_RX_EXTERNAL_INTERRUPT) && ((flags & GPIO_INT_ENABLE) != 0)) {
		return -ENOTSUP;
	}

	pincfg.port_num = cfg->port_num;
	pincfg.pin_num = pin;

	/* Set pull-up if requested */
	if ((flags & GPIO_PULL_UP) != 0) {
		pincfg.cfg.bias_pull_up = 1;
	}

	/* Open drain (pins 0-3: odr0, pins 4-8: odr1) */
	if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
		pincfg.cfg.drive_open_drain = 1;
	}

	/* Set to output */
	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			pincfg.cfg.output_high = 1;
		}
		pincfg.cfg.pin_mode = 0;
		pincfg.cfg.output_enable = 1;
	}

	return pinctrl_configure_pins(&pincfg, 1, PINCTRL_REG_NONE);
}

static int gpio_rx_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_rx_config *cfg = dev->config;

	*value = *(cfg->reg.pidr);
	return 0;
}

static int gpio_rx_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) = ((*cfg->reg.podr) & ~mask) | (mask & value);
	return 0;
}

static int gpio_rx_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) |= pins;
	return 0;
}

static int gpio_rx_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) &= ~pins;
	return 0;
}

static int gpio_rx_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_rx_config *cfg = dev->config;

	*(cfg->reg.podr) = *(cfg->reg.podr) ^ pins;
	return 0;
}

#ifdef CONFIG_RENESAS_RX_EXTERNAL_INTERRUPT
static int gpio_rx_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					   enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rx_config *cfg = dev->config;
	struct gpio_rx_data *data = dev->data;
	int ret = 0;
	k_spinlock_key_t key;
	const struct gpio_rx_irq_info *irq_info = query_irq_info(dev, pin);

	if (irq_info == NULL) {
		return -EINVAL;
	}

	if (!device_is_ready(irq_info->port_irq)) {
		return -EWOULDBLOCK;
	}

	key = k_spin_lock(&data->lock);

	if ((mode & GPIO_INT_ENABLE) != 0) {
		struct gpio_rx_callback callback = {
			.port = (struct device *)dev,
			.port_num = cfg->port_num,
			.pin = pin,
			.mode = mode,
			.trigger = trig,
			.isr = gpio_rx_callback_adapter,
		};

		ret = gpio_rx_interrupt_set(irq_info->port_irq, &callback);
		if (ret < 0) {
			goto end;
		}
		mpc_set_irq(dev, pin, 1);
	}

	if ((mode & GPIO_INT_DISABLE) != 0) {
		gpio_rx_interrupt_unset(irq_info->port_irq, cfg->port_num, pin);
		mpc_set_irq(dev, pin, 0);
	}
end:
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int gpio_rx_manage_callback(const struct device *dev, struct gpio_callback *callback,
				   bool set)
{
	struct gpio_rx_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}
#endif

static DEVICE_API(gpio, gpio_rx_drv_api_funcs) = {
	.pin_configure = gpio_rx_pin_configure,
	.port_get_raw = gpio_rx_port_get_raw,
	.port_set_masked_raw = gpio_rx_port_set_masked_raw,
	.port_set_bits_raw = gpio_rx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rx_port_clear_bits_raw,
	.port_toggle_bits = gpio_rx_port_toggle_bits,
#ifdef CONFIG_RENESAS_RX_EXTERNAL_INTERRUPT
	.pin_interrupt_configure = gpio_rx_pin_interrupt_configure,
	.manage_callback = gpio_rx_manage_callback,
#endif
};

#define GPIO_RX_PINS_NAME(n, p, i) CONCAT(DT_STRING_TOKEN_BY_IDX(n, p, i), _pins)
#define GPIO_RX_DO_NOTHING
#define GPIO_RX_DECL_PINS(n, p, i)                                                                 \
	const uint8_t CONCAT(n, ___pins##i[]) = {DT_FOREACH_PROP_ELEM_SEP(                         \
		n, GPIO_RX_PINS_NAME(n, p, i), DT_PROP_BY_IDX, (, GPIO_RX_DO_NOTHING))};

#ifndef CONFIG_RENESAS_RX_EXTERNAL_INTERRUPT
#define GPIO_RX_IRQ_INFO(n, p, i)
#define GPIO_RX_IRQ_STRUCT_INIT(suffix)
#else
#define GPIO_RX_IRQ_INFO(n, p, i)                                                                  \
	{                                                                                          \
		.port_irq = DEVICE_DT_GET_OR_NULL(DT_PHANDLE_BY_IDX(n, port_irqs, i)),             \
		.pins = CONCAT(n, ___pins##i),                                                     \
		.num = ARRAY_SIZE(CONCAT(n, ___pins##i)),                                          \
	},

#define GPIO_RX_IRQ_STRUCT_INIT(suffix)                                                            \
	.irq_info = gpio_rx_irq_info_##suffix,                                                     \
	.irq_info_size = DT_PROP_LEN_OR(DT_NODELABEL(ioport##suffix), port_irq_names, 0),
#endif

#define GPIO_RX_PORT_IRQ_DECL(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, port_irq_names),                                        \
		    (DT_FOREACH_PROP_ELEM(node, port_irq_names, GPIO_RX_DECL_PINS)), ())

#define GPIO_RX_PORT_IRQ_ELEM(node)                                                                \
	COND_CODE_1(DT_NODE_HAS_PROP(node, port_irq_names),                                        \
		    (DT_FOREACH_PROP_ELEM(node, port_irq_names, GPIO_RX_IRQ_INFO)), ())

#define GPIO_DEVICE_INIT(node, port_number, suffix, addr)                                          \
	GPIO_RX_PORT_IRQ_DECL(node);                                                               \
	struct gpio_rx_irq_info gpio_rx_irq_info_##suffix[] = {GPIO_RX_PORT_IRQ_ELEM(node)};       \
	static const struct gpio_rx_config gpio_rx_config_##suffix = {                             \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(8U)},                   \
		.port_num = port_number,                                                           \
		.pinmux = (uint8_t *)DT_REG_ADDR(DT_PROP(node, pinmux)),                           \
		.reg =                                                                             \
			{                                                                          \
				.pdr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PDR),                  \
				.podr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PODR),                \
				.pidr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PIDR),                \
				.pmr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PMR),                  \
				.odr0 = (uint8_t *)DT_REG_ADDR_BY_NAME_OR(node, ODR0, NULL),       \
				.odr1 = (uint8_t *)DT_REG_ADDR_BY_NAME_OR(node, ODR1, NULL),       \
				.pcr = (uint8_t *)DT_REG_ADDR_BY_NAME(node, PCR),                  \
			},                                                                         \
		GPIO_RX_IRQ_STRUCT_INIT(suffix)};                                                  \
	static struct gpio_rx_data gpio_rx_data_##suffix;                                          \
	DEVICE_DT_DEFINE(node, NULL, NULL, &gpio_rx_data_##suffix, &gpio_rx_config_##suffix,       \
			 PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_rx_drv_api_funcs)

#define GPIO_DEVICE_INIT_RX(suffix)                                                                \
	GPIO_DEVICE_INIT(DT_NODELABEL(ioport##suffix),                                             \
			 DT_PROP(DT_NODELABEL(ioport##suffix), port), suffix,                      \
			 DT_REG_ADDR(DT_NODELABEL(ioport##suffix)))

#define GPIO_DEVICE_INIT_RX_IF_OKAY(suffix)                                                        \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(ioport##suffix), okay),                        \
		    (GPIO_DEVICE_INIT_RX(suffix)), ())

GPIO_DEVICE_INIT_RX_IF_OKAY(0);
GPIO_DEVICE_INIT_RX_IF_OKAY(1);
GPIO_DEVICE_INIT_RX_IF_OKAY(2);
GPIO_DEVICE_INIT_RX_IF_OKAY(3);
GPIO_DEVICE_INIT_RX_IF_OKAY(4);
GPIO_DEVICE_INIT_RX_IF_OKAY(5);
GPIO_DEVICE_INIT_RX_IF_OKAY(a);
GPIO_DEVICE_INIT_RX_IF_OKAY(b);
GPIO_DEVICE_INIT_RX_IF_OKAY(c);
GPIO_DEVICE_INIT_RX_IF_OKAY(d);
GPIO_DEVICE_INIT_RX_IF_OKAY(e);
GPIO_DEVICE_INIT_RX_IF_OKAY(h);
GPIO_DEVICE_INIT_RX_IF_OKAY(j);
