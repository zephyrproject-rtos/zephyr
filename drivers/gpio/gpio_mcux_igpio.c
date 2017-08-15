/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_igpio.h>

#include "gpio_utils.h"

struct mcux_igpio_config {
	GPIO_Type *base;
};

struct mcux_igpio_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

static int mcux_igpio_configure(struct device *dev,
			       int access_op, u32_t pin, int flags)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	gpio_pin_config_t pin_config;
	u32_t i;

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	pin_config.direction = ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN)
		? kGPIO_DigitalInput : kGPIO_DigitalOutput;

	pin_config.outputLogic = 0;

	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				pin_config.interruptMode = kGPIO_IntRisingEdge;
			} else if (flags & GPIO_INT_DOUBLE_EDGE) {
				pin_config.interruptMode =
					kGPIO_IntRisingOrFallingEdge;
			} else {
				pin_config.interruptMode =
					kGPIO_IntFallingEdge;
			}
		} else { /* GPIO_INT_LEVEL */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				pin_config.interruptMode = kGPIO_IntHighLevel;
			} else {
				pin_config.interruptMode = kGPIO_IntLowLevel;
			}
		}
	} else {
		pin_config.interruptMode = kGPIO_NoIntmode;
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		GPIO_PinInit(config->base, pin, &pin_config);
	} else {	/* GPIO_ACCESS_BY_PORT */
		for (i = 0; i < 32; i++) {
			GPIO_PinInit(config->base, i, &pin_config);
		}
	}

	return 0;
}

static int mcux_igpio_write(struct device *dev,
			   int access_op, u32_t pin, u32_t value)
{
	const struct mcux_igpio_config *config = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		GPIO_PinWrite(config->base, pin, value);
	} else { /* GPIO_ACCESS_BY_PORT */
		config->base->DR = value;
	}

	return 0;
}

static int mcux_igpio_read(struct device *dev,
			  int access_op, u32_t pin, u32_t *value)
{
	const struct mcux_igpio_config *config = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = GPIO_PinRead(config->base, pin);
	} else { /* GPIO_ACCESS_BY_PORT */
		*value = config->base->DR;
	}

	return 0;
}

static int mcux_igpio_manage_callback(struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct mcux_igpio_data *data = dev->driver_data;

	_gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int mcux_igpio_enable_callback(struct device *dev,
				     int access_op, u32_t pin)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	struct mcux_igpio_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= BIT(pin);
		GPIO_PortEnableInterrupts(config->base, BIT(pin));
	} else {
		data->pin_callback_enables = 0xFFFFFFFF;
		GPIO_PortEnableInterrupts(config->base, 0xFFFFFFFF);
	}

	return 0;
}

static int mcux_igpio_disable_callback(struct device *dev,
				      int access_op, u32_t pin)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	struct mcux_igpio_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		GPIO_PortDisableInterrupts(config->base, BIT(pin));
		data->pin_callback_enables &= ~BIT(pin);
	} else {
		GPIO_PortDisableInterrupts(config->base, 0);
		data->pin_callback_enables = 0;
	}

	return 0;
}

static void mcux_igpio_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct mcux_igpio_config *config = dev->config->config_info;
	struct mcux_igpio_data *data = dev->driver_data;
	u32_t enabled_int, int_flags;

	int_flags = GPIO_PortGetInterruptFlags(config->base);
	enabled_int = int_flags & data->pin_callback_enables;

	_gpio_fire_callbacks(&data->callbacks, dev, enabled_int);

	GPIO_ClearPinsInterruptFlags(config->base, enabled_int);
}

static const struct gpio_driver_api mcux_igpio_driver_api = {
	.config = mcux_igpio_configure,
	.write = mcux_igpio_write,
	.read = mcux_igpio_read,
	.manage_callback = mcux_igpio_manage_callback,
	.enable_callback = mcux_igpio_enable_callback,
	.disable_callback = mcux_igpio_disable_callback,
};

#ifdef CONFIG_GPIO_MCUX_IGPIO_1
static int mcux_igpio_1_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_1_config = {
	.base = (GPIO_Type *)CONFIG_MCUX_IGPIO_1_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_1_data;

DEVICE_AND_API_INIT(mcux_igpio_1, CONFIG_MCUX_IGPIO_1_NAME,
		    mcux_igpio_1_init,
		    &mcux_igpio_1_data, &mcux_igpio_1_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_1_init(struct device *dev)
{
	IRQ_CONNECT(CONFIG_MCUX_IGPIO_1_IRQ_0, CONFIG_MCUX_IGPIO_1_IRQ_0_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_1), 0);

	irq_enable(CONFIG_MCUX_IGPIO_1_IRQ_0);

	IRQ_CONNECT(CONFIG_MCUX_IGPIO_1_IRQ_1, CONFIG_MCUX_IGPIO_1_IRQ_1_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_1), 0);

	irq_enable(CONFIG_MCUX_IGPIO_1_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_1 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_2
static int mcux_igpio_2_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_2_config = {
	.base = (GPIO_Type *)CONFIG_MCUX_IGPIO_2_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_2_data;

DEVICE_AND_API_INIT(mcux_igpio_2, CONFIG_MCUX_IGPIO_2_NAME,
		    mcux_igpio_2_init,
		    &mcux_igpio_2_data, &mcux_igpio_2_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_2_init(struct device *dev)
{
	IRQ_CONNECT(CONFIG_MCUX_IGPIO_2_IRQ_0, CONFIG_MCUX_IGPIO_2_IRQ_0_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_2), 0);

	irq_enable(CONFIG_MCUX_IGPIO_2_IRQ_0);

	IRQ_CONNECT(CONFIG_MCUX_IGPIO_2_IRQ_1, CONFIG_MCUX_IGPIO_2_IRQ_1_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_2), 0);

	irq_enable(CONFIG_MCUX_IGPIO_2_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_2 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_3
static int mcux_igpio_3_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_3_config = {
	.base = (GPIO_Type *)CONFIG_MCUX_IGPIO_3_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_3_data;

DEVICE_AND_API_INIT(mcux_igpio_3, CONFIG_MCUX_IGPIO_3_NAME,
		    mcux_igpio_3_init,
		    &mcux_igpio_3_data, &mcux_igpio_3_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_3_init(struct device *dev)
{
	IRQ_CONNECT(CONFIG_MCUX_IGPIO_3_IRQ_0, CONFIG_MCUX_IGPIO_3_IRQ_0_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_3), 0);

	irq_enable(CONFIG_MCUX_IGPIO_3_IRQ_0);

	IRQ_CONNECT(CONFIG_MCUX_IGPIO_3_IRQ_1, CONFIG_MCUX_IGPIO_3_IRQ_1_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_3), 0);

	irq_enable(CONFIG_MCUX_IGPIO_3_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_3 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_4
static int mcux_igpio_4_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_4_config = {
	.base = (GPIO_Type *)CONFIG_MCUX_IGPIO_4_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_4_data;

DEVICE_AND_API_INIT(mcux_igpio_4, CONFIG_MCUX_IGPIO_4_NAME,
		    mcux_igpio_4_init,
		    &mcux_igpio_4_data, &mcux_igpio_4_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_4_init(struct device *dev)
{
	IRQ_CONNECT(CONFIG_MCUX_IGPIO_4_IRQ_0, CONFIG_MCUX_IGPIO_4_IRQ_0_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_4), 0);

	irq_enable(CONFIG_MCUX_IGPIO_4_IRQ_0);

	IRQ_CONNECT(CONFIG_MCUX_IGPIO_4_IRQ_1, CONFIG_MCUX_IGPIO_4_IRQ_1_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_4), 0);

	irq_enable(CONFIG_MCUX_IGPIO_4_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_4 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_5
static int mcux_igpio_5_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_5_config = {
	.base = (GPIO_Type *)CONFIG_MCUX_IGPIO_5_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_5_data;

DEVICE_AND_API_INIT(mcux_igpio_5, CONFIG_MCUX_IGPIO_5_NAME,
		    mcux_igpio_5_init,
		    &mcux_igpio_5_data, &mcux_igpio_5_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_5_init(struct device *dev)
{
	IRQ_CONNECT(CONFIG_MCUX_IGPIO_5_IRQ_0, CONFIG_MCUX_IGPIO_5_IRQ_0_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_5), 0);

	irq_enable(CONFIG_MCUX_IGPIO_5_IRQ_0);

	IRQ_CONNECT(CONFIG_MCUX_IGPIO_5_IRQ_1, CONFIG_MCUX_IGPIO_5_IRQ_1_PRI,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_5), 0);

	irq_enable(CONFIG_MCUX_IGPIO_5_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_5 */
