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
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/andestech-atcgpio100.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

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
			sys_write32(pin_mask, GPIO_DSET(port));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			sys_write32(pin_mask, GPIO_DCLR(port));
		}

		key = k_spin_lock(&data->lock);

		/* Set channel output */
		port_value = sys_read32(GPIO_DIR(port));
		sys_write32((port_value | pin_mask), GPIO_DIR(port));

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
			sys_write32(DF_DEBOUNCED_SETTING, GPIO_DEBC(port));
			port_value = sys_read32(GPIO_DEBE(port));
			sys_write32((port_value | pin_mask), GPIO_DEBE(port));
		}

		/* Set channel input */
		port_value = sys_read32(GPIO_DIR(port));
		sys_write32((port_value & ~pin_mask), GPIO_DIR(port));

		k_spin_unlock(&data->lock, key);

	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_atcgpio100_port_get_raw(const struct device *port,
					gpio_port_value_t *value)
{
	*value = sys_read32(GPIO_DIN(port));
	return 0;
}

static int gpio_atcgpio100_set_masked_raw(const struct device *port,
					  gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	port_value = sys_read32(GPIO_DOUT(port));
	sys_write32((port_value & ~mask) | (value & mask), GPIO_DOUT(port));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_atcgpio100_set_bits_raw(const struct device *port,
					gpio_port_pins_t pins)
{
	sys_write32(pins, GPIO_DSET(port));
	return 0;
}

static int gpio_atcgpio100_clear_bits_raw(const struct device *port,
					gpio_port_pins_t pins)
{
	sys_write32(pins, GPIO_DCLR(port));
	return 0;
}

static int gpio_atcgpio100_toggle_bits(const struct device *port,
					gpio_port_pins_t pins)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	port_value = sys_read32(GPIO_DOUT(port));
	sys_write32((port_value ^ pins), GPIO_DOUT(port));

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
		port_value = sys_read32(GPIO_INTE(port));
		sys_write32((port_value & ~BIT(pin)), GPIO_INTE(port));

		/* Clear the remain pending interrupt */
		port_value = sys_read32(GPIO_ISTA(port));
		sys_write32(port_value, GPIO_ISTA(port));
	} else {
		/* Set interrupt mode of pin */
		port_value = sys_read32(GPIO_IMD(port, imr_idx));
		SET_GPIO_INT_MODE(port_value, int_mode, ch_idx);
		sys_write32(port_value, GPIO_IMD(port, imr_idx));

		/* Enable interrupt of pin */
		port_value = sys_read32(GPIO_INTE(port));
		sys_write32((port_value | BIT(pin)), GPIO_INTE(port));
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

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_atcgpio100_port_get_dir(const struct device *port,
					gpio_port_pins_t map,
					gpio_port_pins_t *inputs,
					gpio_port_pins_t *outputs)
{
	const struct gpio_atcgpio100_config * const dev_cfg = port->config;
	uint32_t direction = sys_read32(GPIO_DIR(port));

	map &= dev_cfg->common.port_pin_mask;

	if (inputs != NULL) {
		*inputs = map & ~direction;
	}

	if (outputs != NULL) {
		*outputs = map & direction;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static void gpio_atcgpio100_irq_handler(const struct device *port)
{
	struct gpio_atcgpio100_data * const data = port->data;
	uint32_t port_value;

	port_value = sys_read32(GPIO_ISTA(port));
	sys_write32(port_value, GPIO_ISTA(port));

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
	.manage_callback         = gpio_atcgpio100_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction      = gpio_atcgpio100_port_get_dir,
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

static int gpio_atcgpio100_init(const struct device *port)
{
	const struct gpio_atcgpio100_config * const dev_cfg = port->config;

	/* Disable all interrupts */
	sys_write32(BIT_MASK(0), GPIO_INTE(port));

	/* Write 1 to clear interrupt status */
	sys_write32((uint32_t) BIT64_MASK(32), GPIO_ISTA(port));

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
