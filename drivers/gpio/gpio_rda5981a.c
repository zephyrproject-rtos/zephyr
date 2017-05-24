/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <gpio.h>
#include <soc.h>
#include <sys_io.h>
#include <pinmux/rda5981a/pinmux_rda5981a.h>

#include "gpio_utils.h"

#include "soc_registers.h"

/* convenience defines for GPIO */
#define DEV_GPIO_CFG(dev) \
	((const struct gpio_rda5981a_cfg * const)(dev)->config->config_info)
#define DEV_GPIO_DATA(dev) \
	((struct gpio_rda5981a_dev_data * const)(dev)->driver_data)
#define GPIO_STRUCT(dev)					\
	((volatile struct gpio_rda5981a *)(DEV_GPIO_CFG(dev))->gpio_base_addr)

#define GPIO_INT0_EN_MASK	(0x1 << 6)
#define GPIO_INT1_EN_MASK	(0x1 << 7)

struct gpio_rda5981a_cfg {
	uint32_t gpio_base_addr;
};

struct gpio_rda5981a_dev_data {
	sys_slist_t callbacks;
	uint32_t pin_callback_enables;
};

static int gpio_rda5981a_config(struct device *dev,
			    int access_op, uint32_t pin, int flags)
{
	uint32_t direction, gpio_bit;

	volatile struct gpio_rda5981a *gpio = GPIO_STRUCT(dev);

	direction = flags & GPIO_DIR_MASK;

	gpio_bit = GET_PIN(pin);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		/* pull up/down/normal? */
		/* gpio irq set */
		/* set direction */
		gpio->dir &= ~(1 << gpio_bit);
		gpio->dir |= ((direction == GPIO_DIR_IN) ?  1 : 0) << gpio_bit;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_rda5981a_read(struct device *dev,
			  int access_op, uint32_t pin, uint32_t *value)
{
	uint32_t gpio_bit;
	volatile struct gpio_rda5981a *gpio = GPIO_STRUCT(dev);

	gpio_bit = GET_PIN(pin);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (gpio->din  & 1 << gpio_bit) ? 1 : 0;
	} else { /* GPIO_ACCESS_BY_PORT */
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_rda5981a_write(struct device *dev,
			   int access_op, uint32_t pin, uint32_t value)
{
	uint32_t gpio_bit;
	volatile struct gpio_rda5981a *gpio = GPIO_STRUCT(dev);

	gpio_bit = GET_PIN(pin);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			gpio->dout |= 1 << gpio_bit;
		} else {
			gpio->dout &= ~(1 << gpio_bit);
		}
	} else { /* GPIO_ACCESS_BY_PORT */
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_rda5981a_manage_callback(struct device *dev,
				    struct gpio_callback *callback, bool set)
{
	struct gpio_rda5981a_dev_data *data = DEV_GPIO_DATA(dev);

	_gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}


static int gpio_rda5981a_enable_callback(struct device *dev,
				    int access_op, uint32_t pin)
{
	struct gpio_rda5981a_dev_data *data = DEV_GPIO_DATA(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= (1 << pin);
	} else {
		data->pin_callback_enables = 0xFFFFFFFF;
	}

	return 0;
}

static int gpio_rda5981a_disable_callback(struct device *dev,
				     int access_op, uint32_t pin)
{
	struct gpio_rda5981a_dev_data *data = DEV_GPIO_DATA(dev);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables &= ~(1 << pin);
	} else {
		data->pin_callback_enables = 0;
	}

	return 0;
}

/**
 * @brief Handler for port interrupts
 * @param dev Pointer to device structure for driver instance
 *
 * @return N/A
 */
static void gpio_rda5981a_port_isr(void *arg)
{
	uint32_t enabled_int, int_status = 0;
	struct device *dev = arg;
	struct gpio_rda5981a_dev_data *data = DEV_GPIO_DATA(dev);
	volatile struct gpio_rda5981a *gpio = GPIO_STRUCT(dev);

	int_status = (gpio->intctrl & GPIO_INT0_EN_MASK);
	enabled_int = int_status & data->pin_callback_enables;

	irq_disable(GPIO_IRQ);

	/* Call the registered callbacks */
	_gpio_fire_callbacks(&data->callbacks, (struct device *)dev,
			     enabled_int);

	irq_enable(GPIO_IRQ);
}

static const struct gpio_driver_api gpio_rda5981a_drv_api_funcs = {
	.config = gpio_rda5981a_config,
	.read = gpio_rda5981a_read,
	.write = gpio_rda5981a_write,
	.manage_callback = gpio_rda5981a_manage_callback,
	.enable_callback = gpio_rda5981a_enable_callback,
	.disable_callback = gpio_rda5981a_disable_callback,
};

static const struct gpio_rda5981a_cfg gpio_cfg = {
	.gpio_base_addr   = RDA_GPIO_BASE,
};

static struct gpio_rda5981a_dev_data gpio_data;

static int gpio_rda5981a_init(struct device *dev);

DEVICE_AND_API_INIT(gpio_rda5981a, CONFIG_GPIO_RDA5981A_DEV_NAME,
		    gpio_rda5981a_init,
		    &gpio_data, &gpio_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_rda5981a_drv_api_funcs);

#define SPLASH_LED 1
#if SPLASH_LED
#define SPLASH_STACK_SIZE	256
static char __noinit __stack splash_stack[SPLASH_STACK_SIZE];
static void splash(void *arg1, void *arg2, void *arg3)
{
	int toggle = 1;
	struct device *dev = (struct device *)arg1;

	gpio_rda5981a_config(dev, GPIO_ACCESS_BY_PIN, GPIO_PIN2,
				(GPIO_DIR_OUT));
	gpio_rda5981a_config(dev, GPIO_ACCESS_BY_PIN, GPIO_PIN3,
				(GPIO_DIR_OUT));

	while (1) {
		gpio_rda5981a_write(dev, GPIO_ACCESS_BY_PIN,
					GPIO_PIN2, toggle);
		gpio_rda5981a_write(dev, GPIO_ACCESS_BY_PIN,
					GPIO_PIN3, !toggle);
		k_sleep(1000);
		toggle = toggle ? 0 : 1;
	}
}
#endif
static int gpio_rda5981a_init(struct device *dev)
{
	IRQ_CONNECT(GPIO_IRQ, CONFIG_GPIO_RDA5981A_PORT_PRI,
		    gpio_rda5981a_port_isr, DEVICE_GET(gpio_rda5981a), 0);

	irq_enable(GPIO_IRQ);

#if SPLASH_LED
	k_thread_spawn(&splash_stack[0],
		       SPLASH_STACK_SIZE,
		       (k_thread_entry_t)splash,
		       dev, NULL, NULL,
		       K_PRIO_COOP(10), 0, K_MSEC(500));
#endif
	return 0;
}
