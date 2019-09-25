/*
 * Copyright (c) 2019 Brett Witherspoon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <sys/__assert.h>
#include <device.h>
#include <errno.h>
#include <drivers/gpio.h>

#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/ioc.h>
#include <driverlib/prcm.h>

#include "gpio_utils.h"

struct gpio_cc13xx_cc26xx_data {
	sys_slist_t callbacks;
	u32_t pin_callback_enables;
};

static struct gpio_cc13xx_cc26xx_data gpio_cc13xx_cc26xx_data_0;

static int gpio_cc13xx_cc26xx_config(struct device *port, int access_op,
				     u32_t pin, int flags)
{
	u32_t config;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	__ASSERT_NO_MSG(pin < NUM_IO_MAX);

	config = IOC_CURRENT_2MA | IOC_STRENGTH_AUTO | IOC_SLEW_DISABLE |
		 IOC_NO_WAKE_UP;

	if (flags & GPIO_INT) {
		__ASSERT_NO_MSG((flags & GPIO_DIR_MASK) == GPIO_DIR_IN);

		config |= IOC_INT_ENABLE | IOC_INPUT_ENABLE;

		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_DOUBLE_EDGE) {
				config |= IOC_BOTH_EDGES;
			} else if (flags & GPIO_INT_ACTIVE_HIGH) {
				config |= IOC_RISING_EDGE;
			} else { /* GPIO_INT_ACTIVE_LOW */
				config |= IOC_FALLING_EDGE;
			}
		} else {
			return -ENOTSUP;
		}

		config |= (flags & GPIO_INT_DEBOUNCE) ? IOC_HYST_ENABLE :
							IOC_HYST_DISABLE;
	} else {
		config |= IOC_INT_DISABLE | IOC_NO_EDGE | IOC_HYST_DISABLE;
		config |= (flags & GPIO_DIR_MASK) == GPIO_DIR_IN ?
				  IOC_INPUT_ENABLE :
				  IOC_INPUT_DISABLE;
	}

	config |= (flags & GPIO_POL_MASK) == GPIO_POL_INV ? IOC_IOMODE_INV :
							    IOC_IOMODE_NORMAL;

	switch (flags & GPIO_PUD_MASK) {
	case GPIO_PUD_NORMAL:
		config |= IOC_NO_IOPULL;
		break;
	case GPIO_PUD_PULL_UP:
		config |= IOC_IOPULL_UP;
		break;
	case GPIO_PUD_PULL_DOWN:
		config |= IOC_IOPULL_DOWN;
		break;
	default:
		return -EINVAL;
	}

	IOCPortConfigureSet(pin, IOC_PORT_GPIO, config);

	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
		GPIO_setOutputEnableDio(pin, GPIO_OUTPUT_DISABLE);
	} else {
		GPIO_setOutputEnableDio(pin, GPIO_OUTPUT_ENABLE);
	}

	return 0;
}

static int gpio_cc13xx_cc26xx_write(struct device *port, int access_op,
				    u32_t pin, u32_t value)
{
	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		__ASSERT_NO_MSG(pin < NUM_IO_MAX);
		if (value) {
			GPIO_setDio(pin);
		} else {
			GPIO_clearDio(pin);
		}
		break;
	case GPIO_ACCESS_BY_PORT:
		if (value) {
			GPIO_setMultiDio(GPIO_DIO_ALL_MASK);
		} else {
			GPIO_clearMultiDio(GPIO_DIO_ALL_MASK);
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int gpio_cc13xx_cc26xx_read(struct device *port, int access_op,
				   u32_t pin, u32_t *value)
{
	__ASSERT_NO_MSG(value != NULL);

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		__ASSERT_NO_MSG(pin < NUM_IO_MAX);
		*value = GPIO_readDio(pin);
		break;
	case GPIO_ACCESS_BY_PORT:
		*value = GPIO_readMultiDio(GPIO_DIO_ALL_MASK);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int gpio_cc13xx_cc26xx_manage_callback(struct device *port,
					      struct gpio_callback *callback,
					      bool set)
{
	struct gpio_cc13xx_cc26xx_data *data = port->driver_data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_cc13xx_cc26xx_enable_callback(struct device *port,
					      int access_op, u32_t pin)
{
	struct gpio_cc13xx_cc26xx_data *data = port->driver_data;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		__ASSERT_NO_MSG(pin < NUM_IO_MAX);
		data->pin_callback_enables |= (1 << pin);
		break;
	case GPIO_ACCESS_BY_PORT:
		data->pin_callback_enables = 0xFFFFFFFF;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int gpio_cc13xx_cc26xx_disable_callback(struct device *port,
					       int access_op, u32_t pin)
{
	struct gpio_cc13xx_cc26xx_data *data = port->driver_data;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		__ASSERT_NO_MSG(pin < NUM_IO_MAX);
		data->pin_callback_enables &= ~(1 << pin);
		break;
	case GPIO_ACCESS_BY_PORT:
		data->pin_callback_enables = 0U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static u32_t gpio_cc13xx_cc26xx_get_pending_int(struct device *dev)
{
	return GPIO_getEventMultiDio(GPIO_DIO_ALL_MASK);
}

DEVICE_DECLARE(gpio_cc13xx_cc26xx);

static void gpio_cc13xx_cc26xx_isr(void *arg)
{
	struct device *dev = arg;
	struct gpio_cc13xx_cc26xx_data *data = dev->driver_data;

	u32_t status = GPIO_getEventMultiDio(GPIO_DIO_ALL_MASK);
	u32_t enabled = status & data->pin_callback_enables;

	GPIO_clearEventMultiDio(status);

	gpio_fire_callbacks(&data->callbacks, dev, enabled);
}

static int gpio_cc13xx_cc26xx_init(struct device *dev)
{
	struct gpio_cc13xx_cc26xx_data *data = dev->driver_data;

	/* Enable peripheral power domain */
	PRCMPowerDomainOn(PRCM_DOMAIN_PERIPH);

	/* Enable GPIO peripheral */
	PRCMPeripheralRunEnable(PRCM_PERIPH_GPIO);

	/* Load PRCM settings */
	PRCMLoadSet();
	while (!PRCMLoadGet()) {
		continue;
	}

	/* Enable IRQ */
	IRQ_CONNECT(DT_INST_0_TI_CC13XX_CC26XX_GPIO_IRQ_0,
		    DT_INST_0_TI_CC13XX_CC26XX_GPIO_IRQ_0_PRIORITY,
		    gpio_cc13xx_cc26xx_isr, DEVICE_GET(gpio_cc13xx_cc26xx), 0);
	irq_enable(DT_INST_0_TI_CC13XX_CC26XX_GPIO_IRQ_0);

	/* Disable callbacks */
	data->pin_callback_enables = 0;

	/* Peripheral should not be accessed until power domain is on. */
	while (PRCMPowerDomainStatus(PRCM_DOMAIN_PERIPH) !=
	       PRCM_DOMAIN_POWER_ON) {
		continue;
	}

	return 0;
}

static const struct gpio_driver_api gpio_cc13xx_cc26xx_driver_api = {
	.config = gpio_cc13xx_cc26xx_config,
	.write = gpio_cc13xx_cc26xx_write,
	.read = gpio_cc13xx_cc26xx_read,
	.manage_callback = gpio_cc13xx_cc26xx_manage_callback,
	.enable_callback = gpio_cc13xx_cc26xx_enable_callback,
	.disable_callback = gpio_cc13xx_cc26xx_disable_callback,
	.get_pending_int = gpio_cc13xx_cc26xx_get_pending_int
};

DEVICE_AND_API_INIT(gpio_cc13xx_cc26xx, DT_INST_0_TI_CC13XX_CC26XX_GPIO_LABEL,
		    gpio_cc13xx_cc26xx_init, &gpio_cc13xx_cc26xx_data_0, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_cc13xx_cc26xx_driver_api);
