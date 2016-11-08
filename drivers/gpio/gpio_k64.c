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

#include <errno.h>

#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <gpio.h>
#include <soc.h>
#include <sys_io.h>
#include <pinmux/k64/pinmux.h>

#include "gpio_k64.h"
#include "gpio_utils.h"

static int gpio_k64_config(struct device *dev,
			   int access_op, uint32_t pin, int flags)
{
	const struct gpio_k64_config * const cfg = dev->config->config_info;
	uint32_t value;
	uint32_t setting;
	uint8_t i;

	/* check for an invalid pin configuration */
	if (((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) ||
	    ((flags & GPIO_DIR_IN) && (flags & GPIO_DIR_OUT))) {
		return -ENOTSUP;
	}

	/*
	 * Setup direction register:
	 * 0 - pin is input, 1 - pin is output
	 */
	if (access_op == GPIO_ACCESS_BY_PIN) {
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			sys_clear_bit((cfg->gpio_base_addr +
				       GPIO_K64_DIR_OFFSET), pin);
		} else {  /* GPIO_DIR_OUT */
			sys_set_bit((cfg->gpio_base_addr +
				     GPIO_K64_DIR_OFFSET), pin);
		}
	} else {	/* GPIO_ACCESS_BY_PORT */
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			value = 0x0;
		} else {  /* GPIO_DIR_OUT */
			value = 0xFFFFFFFF;
		}

		sys_write32(value, (cfg->gpio_base_addr + GPIO_K64_DIR_OFFSET));
	}

	/*
	 * Set up pullup/pulldown configuration, in Port Control module:
	 */
	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
		setting = (K64_PINMUX_PULL_ENABLE | K64_PINMUX_PULL_UP);
	} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
		setting = (K64_PINMUX_PULL_ENABLE | K64_PINMUX_PULL_DN);
	} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_NORMAL) {
		setting = K64_PINMUX_PULL_DISABLE;
	} else {
		return -ENOTSUP;
	}

	/*
	 * Set up interrupt configuration, in Port Control module:
	 */
	if (flags & GPIO_INT) {
		/* edge or level */
		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				setting |= K64_PINMUX_INT_RISING;
			} else if (flags & GPIO_INT_DOUBLE_EDGE) {
				setting |= K64_PINMUX_INT_BOTH_EDGE;
			} else {
				setting |= K64_PINMUX_INT_FALLING;
			}
		} else { /* GPIO_INT_LEVEL */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				setting |= K64_PINMUX_INT_HIGH;
			} else {
				setting |= K64_PINMUX_INT_LOW;
			}
		}
	}

	/* write pull-up/-down and, if set, interrupt configuration settings */
	if (access_op == GPIO_ACCESS_BY_PIN) {
		value = sys_read32((cfg->port_base_addr +
				    K64_PINMUX_CTRL_OFFSET(pin)));

		/* clear, then set configuration values */
		value &= ~(K64_PINMUX_PULL_EN_MASK | K64_PINMUX_PULL_SEL_MASK);

		if (flags & GPIO_INT) {
			value &= ~K64_PINMUX_INT_MASK;
		}

		/* Pins must configured as gpio */
		value |= (setting | K64_PINMUX_FUNC_GPIO);

		sys_write32(value, (cfg->port_base_addr +
				    K64_PINMUX_CTRL_OFFSET(pin)));
	} else {  /* GPIO_ACCESS_BY_PORT */
		for (i = 0; i < K64_PINMUX_NUM_PINS; i++) {
			/* clear, then set configuration values */
			value = sys_read32((cfg->port_base_addr +
					    K64_PINMUX_CTRL_OFFSET(i)));

			value &= ~(K64_PINMUX_PULL_EN_MASK |
				   K64_PINMUX_PULL_SEL_MASK);

			if (flags & GPIO_INT) {
				value &= ~K64_PINMUX_INT_MASK;
			}

			/* Pins must configured as gpio */
			value |= (setting | K64_PINMUX_FUNC_GPIO);

			sys_write32(value, (cfg->port_base_addr +
					    K64_PINMUX_CTRL_OFFSET(i)));
		}
	}

	return 0;
}


static int gpio_k64_write(struct device *dev,
			  int access_op, uint32_t pin, uint32_t value)
{
	const struct gpio_k64_config * const cfg = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			sys_set_bit((cfg->gpio_base_addr +
				     GPIO_K64_DATA_OUT_OFFSET), pin);
		} else {
			sys_clear_bit((cfg->gpio_base_addr +
				       GPIO_K64_DATA_OUT_OFFSET), pin);
		}
	} else { /* GPIO_ACCESS_BY_PORT */
		sys_write32(value, (cfg->gpio_base_addr +
				    GPIO_K64_DATA_OUT_OFFSET));
	}

	return 0;
}


static int gpio_k64_read(struct device *dev,
			 int access_op, uint32_t pin, uint32_t *value)
{
	const struct gpio_k64_config * const cfg = dev->config->config_info;

	*value = sys_read32((cfg->gpio_base_addr + GPIO_K64_DATA_IN_OFFSET));

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (*value & BIT(pin)) >> pin;
	}

	/* nothing more to do for GPIO_ACCESS_BY_PORT */

	return 0;
}


static int gpio_k64_manage_callback(struct device *dev,
				    struct gpio_callback *callback, bool set)
{
	struct gpio_k64_data *data = dev->driver_data;

	_gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}


static int gpio_k64_enable_callback(struct device *dev,
				    int access_op, uint32_t pin)
{
	struct gpio_k64_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= BIT(pin);
	} else {
		data->pin_callback_enables = 0xFFFFFFFF;
	}

	return 0;
}


static int gpio_k64_disable_callback(struct device *dev,
				     int access_op, uint32_t pin)
{
	struct gpio_k64_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables &= ~BIT(pin);
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
static void gpio_k64_port_isr(void *dev)
{
	struct device *port = (struct device *)dev;
	struct gpio_k64_data *data = port->driver_data;
	const struct gpio_k64_config *config = port->config->config_info;
	mem_addr_t int_status_reg_addr;
	uint32_t enabled_int, int_status;

	int_status_reg_addr = config->port_base_addr +
		CONFIG_PORT_K64_INT_STATUS_OFFSET;

	int_status = sys_read32(int_status_reg_addr);
	enabled_int = int_status & data->pin_callback_enables;

	_gpio_fire_callbacks(&data->callbacks, port, enabled_int);

	/* clear the port interrupts */
	sys_write32(0xFFFFFFFF, int_status_reg_addr);
}


static const struct gpio_driver_api gpio_k64_drv_api_funcs = {
	.config = gpio_k64_config,
	.write = gpio_k64_write,
	.read = gpio_k64_read,
	.manage_callback = gpio_k64_manage_callback,
	.enable_callback = gpio_k64_enable_callback,
	.disable_callback = gpio_k64_disable_callback,
};

/* Initialization for Port A */
#ifdef CONFIG_GPIO_K64_A

static int gpio_k64_A_init(struct device *dev);

static const struct gpio_k64_config gpio_k64_A_cfg = {
	.gpio_base_addr = GPIO_K64_A_BASE_ADDR,
	.port_base_addr = PORT_K64_A_BASE_ADDR,
};

static struct gpio_k64_data gpio_data_A;

DEVICE_AND_API_INIT(gpio_k64_A, CONFIG_GPIO_K64_A_DEV_NAME, gpio_k64_A_init,
		    &gpio_data_A, &gpio_k64_A_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_k64_drv_api_funcs);

static int gpio_k64_A_init(struct device *dev)
{
	IRQ_CONNECT(GPIO_K64_A_IRQ, CONFIG_GPIO_K64_PORTA_PRI,
		    gpio_k64_port_isr, DEVICE_GET(gpio_k64_A), 0);

	irq_enable(GPIO_K64_A_IRQ);

	return 0;
}

#endif /* CONFIG_GPIO_K64_A */

/* Initialization for Port B */
#ifdef CONFIG_GPIO_K64_B

static int gpio_k64_B_init(struct device *dev);

static const struct gpio_k64_config gpio_k64_B_cfg = {
	.gpio_base_addr = GPIO_K64_B_BASE_ADDR,
	.port_base_addr = PORT_K64_B_BASE_ADDR,
};

static struct gpio_k64_data gpio_data_B;

DEVICE_AND_API_INIT(gpio_k64_B, CONFIG_GPIO_K64_B_DEV_NAME, gpio_k64_B_init,
		    &gpio_data_B, &gpio_k64_B_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_k64_drv_api_funcs);

static int gpio_k64_B_init(struct device *dev)
{
	IRQ_CONNECT(GPIO_K64_B_IRQ, CONFIG_GPIO_K64_PORTB_PRI,
		    gpio_k64_port_isr, DEVICE_GET(gpio_k64_B), 0);

	irq_enable(GPIO_K64_B_IRQ);

	return 0;
}

#endif /* CONFIG_GPIO_K64_B */

/* Initialization for Port C */
#ifdef CONFIG_GPIO_K64_C

static int gpio_k64_C_init(struct device *dev);

static const struct gpio_k64_config gpio_k64_C_cfg = {
	.gpio_base_addr = GPIO_K64_C_BASE_ADDR,
	.port_base_addr = PORT_K64_C_BASE_ADDR,
};

static struct gpio_k64_data gpio_data_C;

DEVICE_AND_API_INIT(gpio_k64_C, CONFIG_GPIO_K64_C_DEV_NAME, gpio_k64_C_init,
		    &gpio_data_C, &gpio_k64_C_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_k64_drv_api_funcs);

static int gpio_k64_C_init(struct device *dev)
{
	IRQ_CONNECT(GPIO_K64_C_IRQ, CONFIG_GPIO_K64_PORTC_PRI,
		    gpio_k64_port_isr, DEVICE_GET(gpio_k64_C), 0);

	irq_enable(GPIO_K64_C_IRQ);

	return 0;
}

#endif /* CONFIG_GPIO_K64_C */

/* Initialization for Port D */
#ifdef CONFIG_GPIO_K64_D

static int gpio_k64_D_init(struct device *dev);

static const struct gpio_k64_config gpio_k64_D_cfg = {
	.gpio_base_addr = GPIO_K64_D_BASE_ADDR,
	.port_base_addr = PORT_K64_D_BASE_ADDR,
};

static struct gpio_k64_data gpio_data_D;

DEVICE_AND_API_INIT(gpio_k64_D, CONFIG_GPIO_K64_D_DEV_NAME, gpio_k64_D_init,
		    &gpio_data_D, &gpio_k64_D_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_k64_drv_api_funcs);

static int gpio_k64_D_init(struct device *dev)
{
	IRQ_CONNECT(GPIO_K64_D_IRQ, CONFIG_GPIO_K64_PORTD_PRI,
		    gpio_k64_port_isr, DEVICE_GET(gpio_k64_D), 0);

	irq_enable(GPIO_K64_D_IRQ);

	return 0;
}

#endif /* CONFIG_GPIO_K64_D */

/* Initialization for Port E */
#ifdef CONFIG_GPIO_K64_E

static int gpio_k64_E_init(struct device *dev);

static const struct gpio_k64_config gpio_k64_E_cfg = {
	.gpio_base_addr = GPIO_K64_E_BASE_ADDR,
	.port_base_addr = PORT_K64_E_BASE_ADDR,
};

static struct gpio_k64_data gpio_data_E;

DEVICE_AND_API_INIT(gpio_k64_E, CONFIG_GPIO_K64_E_DEV_NAME, gpio_k64_E_init,
		    &gpio_data_E, &gpio_k64_E_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_k64_drv_api_funcs);

static int gpio_k64_E_init(struct device *dev)
{
	IRQ_CONNECT(GPIO_K64_E_IRQ, CONFIG_GPIO_K64_PORTE_PRI,
		    gpio_k64_port_isr, DEVICE_GET(gpio_k64_E), 0);

	irq_enable(GPIO_K64_E_IRQ);

	return 0;
}

#endif /* CONFIG_GPIO_K64_E */
