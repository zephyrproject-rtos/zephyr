/*
 * Copyright 2020 Broadcom
 * Copyright 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_iproc_gpio

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>

#define IPROC_GPIO_DATA_IN_OFFSET   0x00
#define IPROC_GPIO_DATA_OUT_OFFSET  0x04
#define IPROC_GPIO_OUT_EN_OFFSET    0x08
#define IPROC_GPIO_INT_TYPE_OFFSET  0x0c
#define IPROC_GPIO_INT_DE_OFFSET    0x10
#define IPROC_GPIO_INT_EDGE_OFFSET  0x14
#define IPROC_GPIO_INT_MSK_OFFSET   0x18
#define IPROC_GPIO_INT_STAT_OFFSET  0x1c
#define IPROC_GPIO_INT_MSTAT_OFFSET 0x20
#define IPROC_GPIO_INT_CLR_OFFSET   0x24
#define IPROC_GPIO_PAD_RES_OFFSET   0x34
#define IPROC_GPIO_RES_EN_OFFSET    0x38

struct gpio_iproc_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	mem_addr_t base;
	void (*irq_config_func)(const struct device *dev);
};

struct gpio_iproc_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

#define DEV_CFG(dev)  ((const struct gpio_iproc_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_iproc_data *const)(dev)->data)

static int gpio_iproc_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;

	/* Setup the pin direcion. */
	if (flags & GPIO_OUTPUT) {
		/* configure pin for output */
		sys_set_bit(base + IPROC_GPIO_OUT_EN_OFFSET, pin);
	} else if (flags & GPIO_INPUT) {
		/* configure pin for input */
		sys_clear_bit(base + IPROC_GPIO_OUT_EN_OFFSET, pin);
	}

	return 0;
}

static int gpio_iproc_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;

	*value = sys_read32(base + IPROC_GPIO_DATA_IN_OFFSET);

	return 0;
}

static int gpio_iproc_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;

	value = sys_read32(base + IPROC_GPIO_DATA_OUT_OFFSET);
	value = (value & (~mask)) | (value & mask);
	sys_write32(value, base + IPROC_GPIO_DATA_OUT_OFFSET);

	return 0;
}

static int gpio_iproc_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;

	sys_write32(mask, base + IPROC_GPIO_DATA_OUT_OFFSET);

	return 0;
}

static int gpio_iproc_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	uint32_t value;
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;

	/* Clear pins. */
	value = sys_read32(base + IPROC_GPIO_DATA_OUT_OFFSET);
	value = (value & ~mask);
	sys_write32(value, base + IPROC_GPIO_DATA_OUT_OFFSET);

	return 0;
}

static int gpio_iproc_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	uint32_t value;
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;

	/* toggles pins. */
	value = sys_read32(base + IPROC_GPIO_DATA_OUT_OFFSET);
	value = (value ^ mask);
	sys_write32(value, base + IPROC_GPIO_DATA_OUT_OFFSET);

	return 0;
}

static int gpio_iproc_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;

	/* check for interrupt configurations */
	if (mode & GPIO_INT_ENABLE) {
		if (mode & GPIO_INT_EDGE) {
			sys_clear_bit(base + IPROC_GPIO_INT_TYPE_OFFSET, pin);
		} else {
			sys_set_bit(base + IPROC_GPIO_INT_TYPE_OFFSET, pin);
		}

		/* Generate interrupt of both falling/rising edge */
		if (trig & GPIO_INT_EDGE_BOTH) {
			sys_set_bit(base + IPROC_GPIO_INT_DE_OFFSET, pin);
		} else if (trig & GPIO_INT_HIGH_1) {
			/* Generate interrupt on rising edge */
			sys_clear_bit(base + IPROC_GPIO_INT_DE_OFFSET, pin);
			sys_set_bit(base + IPROC_GPIO_INT_EDGE_OFFSET, pin);
		} else if (trig & GPIO_INT_LOW_0) {
			/* Generate interrupt on falling edge */
			sys_clear_bit(base + IPROC_GPIO_INT_DE_OFFSET, pin);
			sys_clear_bit(base + IPROC_GPIO_INT_EDGE_OFFSET, pin);
		}

		/* Unmask the interrupt */
		sys_clear_bit(base + IPROC_GPIO_INT_MSTAT_OFFSET, pin);
	} else {
		sys_set_bit(base + IPROC_GPIO_INT_MSK_OFFSET, pin);
	}
	return 0;
}

static void gpio_iproc_isr(const struct device *dev)
{
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);
	mem_addr_t base = cfg->base;
	struct gpio_iproc_data *context = dev->data;
	uint32_t int_stat;

	int_stat = sys_read32(base + IPROC_GPIO_INT_STAT_OFFSET);

	/* Clear the source of the interrupt */
	sys_write32(int_stat, base + IPROC_GPIO_INT_CLR_OFFSET);

	/* Handle the interrupt */
	gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_iproc_manage_callback(const struct device *port, struct gpio_callback *callback,
				      bool set)
{
	struct gpio_iproc_data *context = port->data;

	return gpio_manage_callback(&context->cb, callback, set);
}

static const struct gpio_driver_api gpio_iproc_api = {
	.pin_configure = gpio_iproc_configure,
	.port_get_raw = gpio_iproc_port_get_raw,
	.port_set_masked_raw = gpio_iproc_port_set_masked_raw,
	.port_set_bits_raw = gpio_iproc_port_set_bits_raw,
	.port_clear_bits_raw = gpio_iproc_port_clear_bits_raw,
	.port_toggle_bits = gpio_iproc_port_toggle_bits,
	.pin_interrupt_configure = gpio_iproc_pin_interrupt_configure,
	.manage_callback = gpio_iproc_manage_callback,
};

int gpio_iproc_init(const struct device *dev)
{
	const struct gpio_iproc_config *const cfg = DEV_CFG(dev);

	cfg->irq_config_func(dev);

	return 0;
}

#define GPIO_IPROC_INIT(n)                                                                         \
	static void port_iproc_config_func_##n(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_iproc_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct gpio_iproc_config gpio_port_config_##n = {                             \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = port_iproc_config_func_##n,                                     \
	};                                                                                         \
                                                                                                   \
	static struct gpio_iproc_data gpio_port_data_##n;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_iproc_init, NULL, &gpio_port_data_##n,                       \
			      &gpio_port_config_##n, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,       \
			      &gpio_iproc_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_IPROC_INIT)
