/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mpfs_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/gpio/gpio_utils.h>


#define MSS_GPIO_INPUT_MODE               0x02
#define MSS_GPIO_OUTPUT_MODE              0x05
#define MSS_GPIO_INOUT_MODE               0x03
#define MSS_GPIO_IRQ_LEVEL_HIGH           0x00
#define MSS_GPIO_IRQ_LEVEL_LOW            0x20
#define MSS_GPIO_IRQ_EDGE_POSITIVE        0x40
#define MSS_GPIO_IRQ_EDGE_NEGATIVE        0x60
#define MSS_GPIO_IRQ_EDGE_BOTH            0x80
#define MSS_GPIO_INT_ENABLE_MASK          0x08
#define MSS_OUTPUT_BUFFER_ENABLE_MASK     0x04

struct mss_gpio_t {
	uint32_t gpio_cfg[32];
	uint32_t gpio_irq;
	const uint32_t gpio_in;
	uint32_t gpio_out;
	uint32_t gpio_cfg_all;
	uint32_t gpio_cfg_byte[4];
	uint32_t gpio_clr_bits;
	uint32_t gpio_set_bits;

};

typedef void (*mss_gpio_cfg_func_t)(void);


struct mss_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uintptr_t            gpio_base_addr;
	uint32_t                gpio_irq_base;
	mss_gpio_cfg_func_t    gpio_cfg_func;
};

struct mss_gpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
};


/* Helper Macros for GPIO */
#define DEV_GPIO_CFG(dev)	\
	((const struct mss_gpio_config * const)(dev)->config)
#define DEV_GPIO(dev)	\
	((volatile struct mss_gpio_t *)(DEV_GPIO_CFG(dev))->gpio_base_addr)
#define DEV_GPIO_DATA(dev)	\
	((struct mss_gpio_data *)(dev)->data)


static int mss_gpio_config(const struct device *dev,
					gpio_pin_t pin,
					gpio_flags_t flags)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);
	uint32_t io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);

	if (io_flags == GPIO_DISCONNECTED) {

		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {

		gpio->gpio_cfg[pin] |= MSS_GPIO_OUTPUT_MODE;

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio->gpio_out |= BIT(pin);

		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio->gpio_out &= ~BIT(pin);

		}

	} else if (flags & GPIO_INPUT) {
		gpio->gpio_cfg[pin] |= MSS_GPIO_INPUT_MODE;

	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int mss_gpio_port_toggle_bits(const struct device *dev,
					gpio_port_pins_t mask)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);

	gpio->gpio_out ^= mask;

	return 0;
}

static int mss_gpio_port_get_raw(const struct device *dev,
					gpio_port_value_t *value)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);

	*value = gpio->gpio_in;

	return 0;
}

static int mss_gpio_port_set_masked_raw(const struct device *dev,
					   gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);

	gpio->gpio_out = (gpio->gpio_out & ~mask) | (value & mask);

	return 0;
}

static int mss_gpio_port_set_bits_raw(const struct device *dev,
					 gpio_port_pins_t mask)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);

	gpio->gpio_out |= mask;

	return 0;
}

static int mss_gpio_port_clear_bits_raw(const struct device *dev,
					   gpio_port_pins_t mask)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);

	gpio->gpio_out &= ~mask;

	return 0;
}

static int mss_gpio_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	ARG_UNUSED(trig);
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);

	gpio->gpio_cfg[pin] |= (MSS_GPIO_INT_ENABLE_MASK);

	switch (mode | trig) {
	case GPIO_INT_EDGE_BOTH:
		gpio->gpio_cfg[pin] |= (MSS_GPIO_IRQ_EDGE_BOTH);
		break;
	case GPIO_INT_EDGE_RISING:
		gpio->gpio_cfg[pin] |= (MSS_GPIO_IRQ_EDGE_POSITIVE);
		break;
	case GPIO_INT_EDGE_FALLING:
		gpio->gpio_cfg[pin] |= (MSS_GPIO_IRQ_EDGE_NEGATIVE);
		break;
	case GPIO_INT_LEVEL_LOW:
		gpio->gpio_cfg[pin] |= (MSS_GPIO_IRQ_LEVEL_LOW);
		break;
	case GPIO_INT_LEVEL_HIGH:
		gpio->gpio_cfg[pin] |= (MSS_GPIO_IRQ_LEVEL_HIGH);
		break;
	default:
		gpio->gpio_cfg[pin] &= ~MSS_GPIO_INT_ENABLE_MASK;
		break;
	}
	return 0;
}


static int mss_gpio_manage_callback(const struct device *dev,
					   struct gpio_callback *callback,
					   bool set)
{
	struct mss_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}
static const struct gpio_driver_api mss_gpio_driver = {
	.pin_configure           = mss_gpio_config,
	.port_toggle_bits        = mss_gpio_port_toggle_bits,
	.port_get_raw            = mss_gpio_port_get_raw,
	.port_set_masked_raw     = mss_gpio_port_set_masked_raw,
	.port_set_bits_raw       = mss_gpio_port_set_bits_raw,
	.port_clear_bits_raw     = mss_gpio_port_clear_bits_raw,
	.pin_interrupt_configure = mss_gpio_pin_interrupt_configure,
	.manage_callback         = mss_gpio_manage_callback
};


static int mss_gpio_init(const struct device *dev)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);

	gpio->gpio_irq = 0xFFFFFFFFU;

	const struct mss_gpio_config *cfg = DEV_GPIO_CFG(dev);
	/* Configure GPIO device */
	cfg->gpio_cfg_func();
	return 0;
}

static void mss_gpio_irq_handler(const struct device *dev)
{
	volatile struct mss_gpio_t *gpio = DEV_GPIO(dev);
	uint32_t interrupt_status = gpio->gpio_irq;

	gpio->gpio_irq = gpio->gpio_irq;
	struct mss_gpio_data *data = dev->data;

	gpio_fire_callbacks(&data->cb, dev, interrupt_status);
}

#define MSS_GPIO_INIT(n)	\
	static struct mss_gpio_data mss_gpio_data_##n;	\
	static void gpio_mss_gpio_cfg_func_##n(void);	\
	\
	static const struct mss_gpio_config mss_gpio_config_##n = {	\
		.common = {	\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},	\
		.gpio_base_addr = DT_INST_REG_ADDR(n),	\
		.gpio_irq_base  = DT_INST_IRQN(n),	\
		.gpio_cfg_func = gpio_mss_gpio_cfg_func_##n	\
	};	\
	\
	DEVICE_DT_INST_DEFINE(n,	\
				mss_gpio_init,	\
				NULL,	\
				&mss_gpio_data_##n, &mss_gpio_config_##n,	\
				PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,	\
				&mss_gpio_driver);	\
	\
	static void gpio_mss_gpio_cfg_func_##n(void)	\
	{	\
		IRQ_CONNECT(DT_INST_IRQN(n),	\
				DT_INST_IRQ(n, priority),	\
				mss_gpio_irq_handler,	\
				DEVICE_DT_INST_GET(n),	\
				0);	\
		irq_enable(DT_INST_IRQN(n));	\
	}	\

DT_INST_FOREACH_STATUS_OKAY(MSS_GPIO_INIT)
