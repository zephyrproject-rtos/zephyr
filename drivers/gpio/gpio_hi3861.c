/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hisilicon_hi3861_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl/pinctrl_hi3861.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/irq.h>

#define GPIO_BASE DT_INST_REG_ADDR(0)

#define GPIO_SWPORT_DR     (GPIO_BASE + 0x00)
#define GPIO_SWPORT_DDR    (GPIO_BASE + 0x04)
#define GPIO_INTEN         (GPIO_BASE + 0x30)
#define GPIO_INTMASK       (GPIO_BASE + 0x34)
#define GPIO_INTTYPE_LEVEL (GPIO_BASE + 0x38)
#define GPIO_INT_POLARITY  (GPIO_BASE + 0x3c)
#define GPIO_INTSTATUS     (GPIO_BASE + 0x40)
#define GPIO_RAWINTSTATUS  (GPIO_BASE + 0x44)
#define GPIO_PORT_EOI      (GPIO_BASE + 0x4c)
#define GPIO_EXT_PORT      (GPIO_BASE + 0x50)

struct gpio_hi3861_config {
	struct gpio_driver_config common;
};

struct gpio_hi3861_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
};

static int gpio_hi3861_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	uint32_t regval;

	ARG_UNUSED(port);

	if (flags & GPIO_INPUT) {
#if CONFIG_PINCTRL_HI3861
		if (flags & GPIO_PULL_UP) {
			pinctrl_hi3861_set_pullup(pin, 1);
		} else if (flags & GPIO_PULL_DOWN) {
			pinctrl_hi3861_set_pulldown(pin, 1);
		}
#else
		if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
			return -ENOTSUP;
		}
#endif

		regval = sys_read32(GPIO_SWPORT_DDR);
		regval &= ~BIT(pin);
		sys_write32(regval, GPIO_SWPORT_DDR);
	} else if (flags & GPIO_OUTPUT) {
		regval = sys_read32(GPIO_SWPORT_DDR);
		regval |= BIT(pin);
		sys_write32(regval, GPIO_SWPORT_DDR);

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			regval = sys_read32(GPIO_SWPORT_DR);
			regval |= BIT(pin);
			sys_write32(regval, GPIO_SWPORT_DR);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			regval = sys_read32(GPIO_SWPORT_DR);
			regval &= ~BIT(pin);
			sys_write32(regval, GPIO_SWPORT_DR);
		}
	}

	return 0;
}

static int gpio_hi3861_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	ARG_UNUSED(port);

	*value = sys_read32(GPIO_EXT_PORT);

	return 0;
}

static int gpio_hi3861_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	uint32_t regval;

	ARG_UNUSED(port);

	regval = sys_read32(GPIO_SWPORT_DR) & ~mask;
	regval |= value & mask;
	sys_write32(regval, GPIO_SWPORT_DR);

	return 0;
}

static int gpio_hi3861_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	uint32_t regval;

	ARG_UNUSED(port);

	regval = sys_read32(GPIO_SWPORT_DR);
	regval |= pins;
	sys_write32(regval, GPIO_SWPORT_DR);

	return 0;
}

static int gpio_hi3861_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	uint32_t regval;

	ARG_UNUSED(port);

	regval = sys_read32(GPIO_SWPORT_DR);
	regval &= ~pins;
	sys_write32(regval, GPIO_SWPORT_DR);

	return 0;
}

static int gpio_hi3861_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	uint32_t regval;

	ARG_UNUSED(port);

	regval = sys_read32(GPIO_SWPORT_DR);
	regval ^= pins;
	sys_write32(regval, GPIO_SWPORT_DR);

	return 0;
}

static int gpio_hi3861_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint32_t regval;

	ARG_UNUSED(port);

	if (trig == GPIO_INT_TRIG_BOTH) {
		return -ENOTSUP;
	}

	/* Disable interrupt first */
	regval = sys_read32(GPIO_INTEN);
	regval &= ~BIT(pin);
	sys_write32(regval, GPIO_INTEN);
	if (mode == GPIO_INT_MODE_DISABLED) {
		return 0;
	}

	regval = sys_read32(GPIO_INTTYPE_LEVEL);
	if (mode == GPIO_INT_MODE_EDGE) {
		regval |= BIT(pin);
	} else {
		regval &= ~BIT(pin);
	}
	sys_write32(regval, GPIO_INTTYPE_LEVEL);

	regval = sys_read32(GPIO_INT_POLARITY);
	if (trig == GPIO_INT_TRIG_HIGH) {
		regval |= BIT(pin);
	} else if (trig == GPIO_INT_TRIG_LOW) {
		regval &= ~BIT(pin);
	}
	sys_write32(regval, GPIO_INT_POLARITY);

	/* Enable interrupt */
	regval = sys_read32(GPIO_INTEN);
	regval |= BIT(pin);
	sys_write32(regval, GPIO_INTEN);

	return 0;
}

static int gpio_hi3861_manage_callback(const struct device *port, struct gpio_callback *cb,
				       bool set)
{
	struct gpio_hi3861_data *data = port->data;

	return gpio_manage_callback(&data->cb, cb, set);
}

static void gpio_hi3861_isr(const struct device *port)
{
	struct gpio_hi3861_data *data = port->data;
	uint32_t status;

	status = sys_read32(GPIO_INTSTATUS);
	gpio_fire_callbacks(&data->cb, port, status);

	/* Clear interrupt */
	sys_write32(status, GPIO_PORT_EOI);
}

int gpio_hi3861_init(const struct device *port)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), gpio_hi3861_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

static const struct gpio_driver_api gpio_hi3861_api = {
	.pin_configure = gpio_hi3861_pin_configure,
	.port_get_raw = gpio_hi3861_port_get_raw,
	.port_set_masked_raw = gpio_hi3861_port_set_masked_raw,
	.port_set_bits_raw = gpio_hi3861_port_set_bits_raw,
	.port_clear_bits_raw = gpio_hi3861_port_clear_bits_raw,
	.port_toggle_bits = gpio_hi3861_port_toggle_bits,
	.pin_interrupt_configure = gpio_hi3861_pin_interrupt_configure,
	.manage_callback = gpio_hi3861_manage_callback,
};

static struct gpio_hi3861_data gpio_hi3861_runtime;

static const struct gpio_hi3861_config gpio_hi3861_cfg = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
	}};

DEVICE_DT_INST_DEFINE(0, gpio_hi3861_init, NULL, &gpio_hi3861_runtime, &gpio_hi3861_cfg,
		      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_hi3861_api);
