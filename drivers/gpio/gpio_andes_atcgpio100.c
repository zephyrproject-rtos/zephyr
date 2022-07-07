/*
 * Copyright (c) 2021 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file GPIO driver for the AndesTech ATCGPIO100 controller
 */

#include <errno.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/andestech-atcgpio100.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#include "gpio_utils.h"

#define DT_DRV_COMPAT andestech_atcgpio100

/* Andes ATCGPIO100 register definition */
#define REG_IDR  0x00 /* ID and Revision reg.           */
#define REG_CFG  0x10 /* Hardware configure reg.        */
#define REG_DIN  0x20 /* Data In reg.                   */
#define REG_DOUT 0x24 /* Data Out reg.                  */
#define REG_DIR  0x28 /* Channel direction reg.         */
#define REG_DCLR 0x2C /* Data out clear reg.            */
#define REG_DSET 0x30 /* Data out set reg.              */
#define REG_PUEN 0x40 /* Pull enable reg.               */
#define REG_PTYP 0x44 /* Pull type reg.                 */
#define REG_INTE 0x50 /* Interrupt enable reg.          */
#define REG_IMD0 0x54 /* Interrupt mode 0 ~ 7 reg.      */
#define REG_IMD1 0x58 /* Interrupt mode 8 ~ 15 reg.     */
#define REG_IMD2 0x5C /* Interrupt mode 16 ~ 23 reg.    */
#define REG_IMD3 0x60 /* Interrupt mode 24 ~ 31 reg.    */
#define REG_ISTA 0x64 /* Interrupt status reg.          */
#define REG_DEBE 0x70 /* De-bounce enable reg.          */
#define REG_DEBC 0x74 /* De-Bounce control reg.         */

#define INT_NO_OPERATION        0x0
#define INT_HIGH_LEVEL          0x2
#define INT_LOW_LEVEL           0x3
#define INT_NEGATIVE_EDGE       0x5
#define INT_POSITIVE_EDGE       0x6
#define INT_DUAL_EDGE           0x7

#define PULL_CONFIGURED         BIT(31)
#define DEBOUNCE_CONFIGURED     BIT(29)
#define DF_DEBOUNCED_SETTING    (0x80000003)

#define GPIO_BASE(dev) \
	((const struct gpio_atcgpio100_config * const)(dev)->config)->base

#define GPIO_CFG(dev)  (GPIO_BASE(dev) + REG_CFG)
#define GPIO_DIR(dev)  (GPIO_BASE(dev) + REG_DIR)
#define GPIO_DIN(dev)  (GPIO_BASE(dev) + REG_DIN)
#define GPIO_DOUT(dev) (GPIO_BASE(dev) + REG_DOUT)
#define GPIO_DCLR(dev) (GPIO_BASE(dev) + REG_DCLR)
#define GPIO_DSET(dev) (GPIO_BASE(dev) + REG_DSET)
#define GPIO_PUEN(dev) (GPIO_BASE(dev) + REG_PUEN)
#define GPIO_PTYP(dev) (GPIO_BASE(dev) + REG_PTYP)
#define GPIO_INTE(dev) (GPIO_BASE(dev) + REG_INTE)
#define GPIO_IMD(dev, idx) (GPIO_BASE(dev) + REG_IMD0 + (idx * 4))
#define GPIO_ISTA(dev) (GPIO_BASE(dev) + REG_ISTA)
#define GPIO_DEBE(dev) (GPIO_BASE(dev) + REG_DEBE)
#define GPIO_DEBC(dev) (GPIO_BASE(dev) + REG_DEBC)

#define INWORD(x)     sys_read32(x)
#define OUTWORD(x, d) sys_write32(d, x)

#define SET_GPIO_INT_MODE(cur_val, mode, ch_idx)			\
		do {							\
			cur_val &= ~(BIT_MASK(3) << (ch_idx * 4));	\
			cur_val |=  (mode << (ch_idx * 4));		\
		} while (false)


typedef void (*atcgpio100_cfg_func_t)(void);

struct gpio_atcgpio100_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t                  base;
	uint32_t                  irq_num;
	atcgpio100_cfg_func_t     cfg_func;
};

struct gpio_atcgpio100_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
	struct k_spinlock lock;
};

static int gpio_atcgpio100_config(const struct device *port,
				  gpio_pin_t pin,
				  gpio_flags_t flags)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value, pin_mask, io_flags;
	k_spinlock_key_t key;

	/* Does not support disconnected pin, and
	 * not supporting both input/output at same time.
	 */
	io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	if ((io_flags == GPIO_DISCONNECTED)
	    || (io_flags == (GPIO_INPUT | GPIO_OUTPUT))) {
		return -ENOTSUP;
	}

	pin_mask = BIT(pin);

	if (flags & GPIO_OUTPUT) {

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			OUTWORD(GPIO_DSET(port), pin_mask);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			OUTWORD(GPIO_DCLR(port), pin_mask);
		}

		key = k_spin_lock(&data->lock);

		/* Set channel output */
		port_value = INWORD(GPIO_DIR(port));
		OUTWORD(GPIO_DIR(port), port_value | pin_mask);

		k_spin_unlock(&data->lock, key);

	} else if (flags & GPIO_INPUT) {

		if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
			return -ENOTSUP;
		}

		key = k_spin_lock(&data->lock);

		/* Set de-bounce */
		if (flags & ATCGPIO100_GPIO_DEBOUNCE) {
			/* Default settings: Filter out pulses which are
			 *  less than 4 de-bounce clock period
			 */
			OUTWORD(GPIO_DEBC(port), DF_DEBOUNCED_SETTING);
			port_value = INWORD(GPIO_DEBE(port));
			OUTWORD(GPIO_DEBE(port), port_value | pin_mask);
		}

		/* Set channel input */
		port_value = INWORD(GPIO_DIR(port));
		OUTWORD(GPIO_DIR(port), port_value & ~pin_mask);

		k_spin_unlock(&data->lock, key);

	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_atcgpio100_port_get_raw(const struct device *port,
					gpio_port_value_t *value)
{
	*value = INWORD(GPIO_DIN(port));
	return 0;
}

static int gpio_atcgpio100_set_masked_raw(const struct device *port,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	port_value = INWORD(GPIO_DOUT(port));
	OUTWORD(GPIO_DOUT(port), (port_value & ~mask) | (value & mask));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_atcgpio100_set_bits_raw(const struct device *port,
					gpio_port_pins_t pins)
{
	OUTWORD(GPIO_DSET(port), pins);
	return 0;
}

static int gpio_atcgpio100_clear_bits_raw(const struct device *port,
					gpio_port_pins_t pins)
{
	OUTWORD(GPIO_DCLR(port), pins);
	return 0;
}

static int gpio_atcgpio100_toggle_bits(const struct device *port,
					gpio_port_pins_t pins)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	port_value = INWORD(GPIO_DOUT(port));
	OUTWORD(GPIO_DOUT(port), port_value ^ pins);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_atcgpio100_pin_interrupt_configure(
					const struct device *port,
					gpio_pin_t pin,
					enum gpio_int_mode mode,
					enum gpio_int_trig trig)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value, int_mode, imr_idx, ch_idx;
	k_spinlock_key_t key;

	switch (mode | trig) {
	case GPIO_INT_EDGE_BOTH:
		int_mode = INT_DUAL_EDGE;
		break;
	case GPIO_INT_EDGE_RISING:
		int_mode = INT_POSITIVE_EDGE;
		break;
	case GPIO_INT_EDGE_FALLING:
		int_mode = INT_NEGATIVE_EDGE;
		break;
	case GPIO_INT_LEVEL_LOW:
		int_mode = INT_LOW_LEVEL;
		break;
	case GPIO_INT_LEVEL_HIGH:
		int_mode = INT_HIGH_LEVEL;
		break;
	default:
		int_mode = INT_NO_OPERATION;
		break;
	}

	imr_idx = (pin / 8);
	ch_idx = (pin % 8);

	key = k_spin_lock(&data->lock);

	if (int_mode == INT_NO_OPERATION) {
		/* Disable interrupt of pin */
		port_value = INWORD(GPIO_INTE(port));
		OUTWORD(GPIO_INTE(port), port_value & ~BIT(pin));

		/* Clear the remain pending interrupt */
		port_value = INWORD(GPIO_ISTA(port));
		OUTWORD(GPIO_ISTA(port), port_value);
	} else {
		/* Set interrupt mode of pin */
		port_value = INWORD(GPIO_IMD(port, imr_idx));
		SET_GPIO_INT_MODE(port_value, int_mode, ch_idx);
		OUTWORD(GPIO_IMD(port, imr_idx), port_value);

		/* Enable interrupt of pin */
		port_value = INWORD(GPIO_INTE(port));
		OUTWORD(GPIO_INTE(port), port_value | BIT(pin));
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_atcgpio100_manage_callback(const struct device *port,
					struct gpio_callback *callback,
					bool set)
{

	struct gpio_atcgpio100_data * const data = port->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static void gpio_atcgpio100_irq_handler(const struct device *port)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value;

	port_value = INWORD(GPIO_ISTA(port));
	OUTWORD(GPIO_ISTA(port), port_value);

	gpio_fire_callbacks(&data->cb, port, port_value);

}

static const struct gpio_driver_api gpio_atcgpio100_api = {
	.pin_configure           = gpio_atcgpio100_config,
	.port_get_raw            = gpio_atcgpio100_port_get_raw,
	.port_set_masked_raw     = gpio_atcgpio100_set_masked_raw,
	.port_set_bits_raw       = gpio_atcgpio100_set_bits_raw,
	.port_clear_bits_raw     = gpio_atcgpio100_clear_bits_raw,
	.port_toggle_bits        = gpio_atcgpio100_toggle_bits,
	.pin_interrupt_configure = gpio_atcgpio100_pin_interrupt_configure,
	.manage_callback         = gpio_atcgpio100_manage_callback
};

static int gpio_atcgpio100_init(const struct device *port)
{
	const struct gpio_atcgpio100_config * const dev_cfg = port->config;

	/* Disable all interrupts */
	OUTWORD(GPIO_INTE(port), BIT_MASK(0));

	/* Write 1 to clear interrupt status */
	OUTWORD(GPIO_ISTA(port), (uint32_t) BIT64_MASK(32));

	/* Configure GPIO device */
	dev_cfg->cfg_func();

	/* Enable PLIC interrupt GPIO source */
	irq_enable(dev_cfg->irq_num);

	return 0;
}

#define GPIO_ATCGPIO100_INIT(n)						\
	static void gpio_atcgpio100_cfg_func_##n(void);			\
	static struct gpio_atcgpio100_data gpio_atcgpio100_data_##n;	\
									\
	static const struct gpio_atcgpio100_config			\
		gpio_atcgpio100_config_##n = {				\
			.common = {					\
				.port_pin_mask =			\
				GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
			},						\
			.base = DT_INST_REG_ADDR(n),			\
			.irq_num = DT_INST_IRQN(n),			\
			.cfg_func = gpio_atcgpio100_cfg_func_##n	\
		};							\
									\
	DEVICE_DT_INST_DEFINE(n,			                \
		gpio_atcgpio100_init,					\
		NULL,							\
		&gpio_atcgpio100_data_##n,				\
		&gpio_atcgpio100_config_##n,				\
		PRE_KERNEL_1,						\
		CONFIG_GPIO_INIT_PRIORITY,				\
		&gpio_atcgpio100_api);					\
									\
	static void gpio_atcgpio100_cfg_func_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    gpio_atcgpio100_irq_handler,		\
			    DEVICE_DT_INST_GET(n),	                \
			    0);						\
		return;							\
	}								\


DT_INST_FOREACH_STATUS_OKAY(GPIO_ATCGPIO100_INIT)
