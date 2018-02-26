/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief GPIO driver for LPC54XXX family
 *
 * Note:
 * - Only basic GPIO features sufficient to blinky functionality
 *   are currently implemented.
 * - Interrupt mode is not implemented.
 */

#include <errno.h>
#include <device.h>
#include <gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include "gpio_utils.h"
#include <fsl_gpio.h>
#include <fsl_device_registers.h>

#define PORT0_IDX	0u
#define PORT1_IDX	1u

struct gpio_mcux_lpc_config {
	GPIO_Type *gpio_base;
	u32_t port_no;
	clock_ip_name_t clock_ip_name;
};

struct gpio_mcux_lpc_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

static int gpio_mcux_lpc_configure(struct device *dev,
				   int access_op, u32_t pin, int flags)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}
	/* Check if GPIO port supports interrupts */
	if (flags & GPIO_INT) {
		return -EINVAL;
	}
	/* supports access by pin now,you can add access by port when needed */
	if (access_op == GPIO_ACCESS_BY_PIN) {
		/* input-0,output-1 */
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			gpio_base->DIR[config->port_no] &= ~(BIT(pin));
		} else {
			gpio_base->SET[config->port_no] = BIT(pin);
			gpio_base->DIR[config->port_no] |= BIT(pin);
		}
	} else {

		return -EINVAL;
	}

	return 0;
}

static int gpio_mcux_lpc_write(struct device *dev,
			       int access_op, u32_t pin, u32_t value)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		/* Set/Clear the data output for the respective pin */
		gpio_base->B[config->port_no][pin] = value;
	} else { /* return an error for all other options */
		return -EINVAL;
	}

	return 0;
}

static int gpio_mcux_lpc_read(struct device *dev,
			      int access_op, u32_t pin, u32_t *value)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PIN[config->port_no];

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (*value & BIT(pin)) >> pin;
	} else { /* return an error for all other options */
		return -EINVAL;
	}

	return 0;
}

static int gpio_mcux_lpc_init(struct device *dev)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;

	CLOCK_EnableClock(config->clock_ip_name);

	return 0;
}

static const struct gpio_driver_api gpio_mcux_lpc_driver_api = {
	.config = gpio_mcux_lpc_configure,
	.write = gpio_mcux_lpc_write,
	.read = gpio_mcux_lpc_read,
};

#ifdef CONFIG_GPIO_MCUX_LPC_PORT0
static const struct gpio_mcux_lpc_config gpio_mcux_lpc_port0_config = {
	.gpio_base = GPIO,
	.port_no = PORT0_IDX,
	.clock_ip_name = kCLOCK_Gpio0,
};

static struct gpio_mcux_lpc_data gpio_mcux_lpc_port0_data;

DEVICE_AND_API_INIT(gpio_mcux_lpc_port0, CONFIG_GPIO_MCUX_LPC_PORT0_NAME,
		    gpio_mcux_lpc_init,
		    &gpio_mcux_lpc_port0_data, &gpio_mcux_lpc_port0_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_lpc_driver_api);
#endif /* CONFIG_GPIO_MCUX_LPC_PORT0 */

#ifdef CONFIG_GPIO_MCUX_LPC_PORT1
static const struct gpio_mcux_lpc_config gpio_mcux_lpc_port1_config = {
	.gpio_base = GPIO,
	.port_no = PORT1_IDX,
	.clock_ip_name = kCLOCK_Gpio1,
};

static struct gpio_mcux_lpc_data gpio_mcux_lpc_port1_data;

DEVICE_AND_API_INIT(gpio_mcux_lpc_port1, CONFIG_GPIO_MCUX_LPC_PORT1_NAME,
		    gpio_mcux_lpc_init,
		    &gpio_mcux_lpc_port1_data, &gpio_mcux_lpc_port1_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_lpc_driver_api);
#endif /* CONFIG_GPIO_MCUX_LPC_PORT1 */
