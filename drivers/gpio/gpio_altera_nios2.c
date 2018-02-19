/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for the Altera Nios-II PIO Core.
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <gpio.h>

#include "gpio_utils.h"
#include "altera_avalon_pio_regs.h"

typedef int (*config_func_t)(struct device *dev, int access_op,
			     u32_t pin, int flags);

/* Configuration data */
struct gpio_nios2_config {
	u32_t pio_base;
	config_func_t config_func;
};

static int gpio_nios2_config_oput_port(struct device *dev, int access_op,
				       u32_t pin, int flags)
{
	if (((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) || (flags & GPIO_INT)) {
		return -ENOTSUP;
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Configure pin or port
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param flags Flags of pin or port
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_nios2_config(struct device *dev, int access_op,
			     u32_t pin, int flags)
{
	const struct gpio_nios2_config *cfg = dev->config->config_info;

	if (cfg->config_func) {
		return cfg->config_func(dev, access_op, pin, flags);
	}

	return 0;
}

/**
 * @brief Set the pin or port output
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value to set (0 or 1)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_nios2_write(struct device *dev, int access_op,
			    u32_t pin, u32_t value)
{
	const struct gpio_nios2_config *cfg = dev->config->config_info;

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		if (value) {
			/* set the pin */
			IOWR_ALTERA_AVALON_PIO_SET_BITS(cfg->pio_base,
								BIT(pin));
		} else {
			/* clear the pin */
			IOWR_ALTERA_AVALON_PIO_CLEAR_BITS(cfg->pio_base,
								BIT(pin));
		}
		break;
	case GPIO_ACCESS_BY_PORT:
		IOWR_ALTERA_AVALON_PIO_DATA(cfg->pio_base, value);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Read the pin or port status
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param pin The pin number
 * @param value Value of input pin(s)
 *
 * @return 0 if successful, failed otherwise
 */
static int gpio_nios2_read(struct device *dev, int access_op,
			   u32_t pin, u32_t *value)
{
	const struct gpio_nios2_config *cfg = dev->config->config_info;

	*value = IORD_ALTERA_AVALON_PIO_DATA(cfg->pio_base);

	switch (access_op) {
	case GPIO_ACCESS_BY_PIN:
		*value = (*value >> pin) & 0x01;
		break;
	case GPIO_ACCESS_BY_PORT:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct gpio_driver_api gpio_nios2_drv_api_funcs = {
	.config = gpio_nios2_config,
	.write = gpio_nios2_write,
	.read = gpio_nios2_read,
};

/* Output only Port */
#ifdef CONFIG_GPIO_ALTERA_NIOS2_OUTPUT
static const struct gpio_nios2_config gpio_nios2_oput_cfg = {
	.pio_base = LED_BASE,
	.config_func = gpio_nios2_config_oput_port,
};

/**
 * @brief Initialization function of PIO
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_nios2_oput_init(struct device *dev)
{
	return 0;
}

DEVICE_AND_API_INIT(gpio_nios2_oput, CONFIG_GPIO_ALTERA_NIOS2_OUTPUT_DEV_NAME,
		    gpio_nios2_oput_init, NULL, &gpio_nios2_oput_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_nios2_drv_api_funcs);

#endif /* CONFIG_GPIO_ALTERA_NIOS2_OUTPUT */
