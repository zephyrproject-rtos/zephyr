/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Driver for the Freescale K64 GPIO module.
 */

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <gpio.h>

#include "gpio_k64.h"


static int gpio_k64_config(struct device *dev, int access_op,
					       uint32_t pin, int flags)
{
	const struct gpio_k64_config * const cfg = dev->config->config_info;
	uint32_t value = 0;

	/*
	 * Setup direction register:
	 * 0 - pin is input, 1 - pin is output
	 */

	if (access_op == GPIO_ACCESS_BY_PIN) {

		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			sys_clear_bit((cfg->base_addr + GPIO_K64_DIR_OFFSET), pin);
		} else {
			sys_set_bit((cfg->base_addr + GPIO_K64_DIR_OFFSET), pin);
		}

	} else {	/* GPIO_ACCESS_BY_PORT */

		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			value = 0x0;
		} else {
			value = 0xFFFFFFFF;
		}
		sys_write32(value, (cfg->base_addr + GPIO_K64_DIR_OFFSET));

	}

	return DEV_OK;
}


static int gpio_k64_write(struct device *dev, int access_op,
							uint32_t pin, uint32_t value)
{
	const struct gpio_k64_config * const cfg = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {

		if (value) {
			sys_set_bit((cfg->base_addr + GPIO_K64_DATA_OUT_OFFSET), pin);
		} else {
			sys_clear_bit((cfg->base_addr + GPIO_K64_DATA_OUT_OFFSET), pin);
		}

	} else {	/* GPIO_ACCESS_BY_PORT */

		sys_write32(value, (cfg->base_addr + GPIO_K64_DATA_OUT_OFFSET));

	}

	return DEV_OK;
}


static int gpio_k64_read(struct device *dev, int access_op,
							uint32_t pin, uint32_t *value)
{
	const struct gpio_k64_config * const cfg = dev->config->config_info;

	*value = sys_read32((cfg->base_addr + GPIO_K64_DATA_IN_OFFSET));

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (*value & (1 << pin)) >> pin;
	}

	/* nothing more to do for GPIO_ACCESS_BY_PORT */

	return DEV_OK;
}


static int gpio_k64_set_callback(struct device *dev, gpio_callback_t callback)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);

	return DEV_INVALID_OP;
}

static int gpio_k64_enable_callback(struct device *dev, int access_op,
									uint32_t pin)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	return DEV_INVALID_OP;
}

static int gpio_k64_disable_callback(struct device *dev, int access_op,
										uint32_t pin)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(pin);

	return DEV_INVALID_OP;
}

static int gpio_k64_suspend_port(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_INVALID_OP;
}

static int gpio_k64_resume_port(struct device *dev)
{
	ARG_UNUSED(dev);

	return DEV_INVALID_OP;
}

static struct gpio_driver_api gpio_k64_drv_api_funcs = {
	.config = gpio_k64_config,
	.write = gpio_k64_write,
	.read = gpio_k64_read,
	.set_callback = gpio_k64_set_callback,
	.enable_callback = gpio_k64_enable_callback,
	.disable_callback = gpio_k64_disable_callback,
	.suspend = gpio_k64_suspend_port,
	.resume = gpio_k64_resume_port,
};


/**
 * @brief Initialization function of Freescale K64-based GPIO port
 *
 * @param dev Device structure pointer
 * @return DEV_OK if successful, failed otherwise.
 */
int gpio_k64_init(struct device *dev)
{
	dev->driver_api = &gpio_k64_drv_api_funcs;

	return DEV_OK;
}

/* Initialization for Port A */
#ifdef CONFIG_GPIO_K64_A

static struct gpio_k64_config gpio_k64_A_cfg = {
	.base_addr = CONFIG_GPIO_K64_A_BASE_ADDR,
};

DEVICE_INIT(gpio_k64_A, CONFIG_GPIO_K64_A_DEV_NAME, gpio_k64_init,
			NULL, &gpio_k64_A_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_GPIO_K64_A */

/* Initialization for Port B */
#ifdef CONFIG_GPIO_K64_B

static struct gpio_k64_config gpio_k64_B_cfg = {
	.base_addr = CONFIG_GPIO_K64_B_BASE_ADDR,
};

DEVICE_INIT(gpio_k64_B, CONFIG_GPIO_K64_B_DEV_NAME, gpio_k64_init,
			NULL, &gpio_k64_B_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_GPIO_K64_B */

/* Initialization for Port C */
#ifdef CONFIG_GPIO_K64_C

static struct gpio_k64_config gpio_k64_C_cfg = {
	.base_addr = CONFIG_GPIO_K64_C_BASE_ADDR,
};

DEVICE_INIT(gpio_k64_C, CONFIG_GPIO_K64_C_DEV_NAME, gpio_k64_init,
			NULL, &gpio_k64_C_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_GPIO_K64_C */

/* Initialization for Port D */
#ifdef CONFIG_GPIO_K64_D

static struct gpio_k64_config gpio_k64_D_cfg = {
	.base_addr = CONFIG_GPIO_K64_D_BASE_ADDR,
};

DEVICE_INIT(gpio_k64_D, CONFIG_GPIO_K64_D_DEV_NAME, gpio_k64_init,
			NULL, &gpio_k64_D_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_GPIO_K64_D */

/* Initialization for Port E */
#ifdef CONFIG_GPIO_K64_E

static struct gpio_k64_config gpio_k64_E_cfg = {
	.base_addr = CONFIG_GPIO_K64_E_BASE_ADDR,
};

DEVICE_INIT(gpio_k64_E, CONFIG_GPIO_K64_E_DEV_NAME, gpio_k64_init,
			NULL, &gpio_k64_E_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_GPIO_K64_E */
