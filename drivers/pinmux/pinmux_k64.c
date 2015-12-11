/* pinmux_k64.c - pin out mapping for the Freescale FRDM-K64F board */

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

#include <stdbool.h>
#include <nanokernel.h>
#include <device.h>
#include <init.h>
#include <sys_io.h>

#include <pinmux.h>
#include <i2c.h>
#include <gpio.h>
#include <pwm.h>

#include <gpio/gpio_k64.h>
#include <pinmux/pinmux.h>
#include <pinmux/pinmux_k64.h>

/* port pin number conversion from pin ID */
#define PIN_FROM_ID(pin_id)	(pin_id % K64_PINMUX_NUM_PINS)

struct fsl_k64_data {
	struct device *gpio_a;  /* port A */
	struct device *gpio_b;  /* port B */
	struct device *gpio_c;  /* port C */
	struct device *gpio_d;  /* port D */
	struct device *gpio_e;  /* port E */
};


#ifdef CONFIG_GPIO_K64_A
static inline int config_port_a(mem_addr_t *addr)
{
	*addr = CONFIG_PORT_K64_A_BASE_ADDR;
	return DEV_OK;
}
#else
#define config_port_a(addr) DEV_NO_ACCESS
#endif

#ifdef CONFIG_GPIO_K64_B
static inline int config_port_b(mem_addr_t *addr)
{
	*addr = CONFIG_PORT_K64_B_BASE_ADDR;
	return DEV_OK;
}
#else
#define config_port_b(addr) DEV_NO_ACCESS
#endif

#ifdef CONFIG_GPIO_K64_C
static inline int config_port_c(mem_addr_t *addr)
{
	*addr = CONFIG_PORT_K64_C_BASE_ADDR;
	return DEV_OK;
}
#else
#define config_port_c(addr) DEV_NO_ACCESS
#endif

#ifdef CONFIG_GPIO_K64_D
static inline int config_port_d(mem_addr_t *addr)
{
	*addr = CONFIG_PORT_K64_D_BASE_ADDR;
	return DEV_OK;
}
#else
#define config_port_d(addr) DEV_NO_ACCESS
#endif

#ifdef CONFIG_GPIO_K64_E
static inline int config_port_e(mem_addr_t *addr)
{
	*addr = CONFIG_PORT_K64_E_BASE_ADDR;
	return DEV_OK;
}
#else
#define config_port_e(addr) DEV_NO_ACCESS
#endif

static int _fsl_k64_get_port_addr(uint8_t pin_id, mem_addr_t *port_addr_ptr)
{

	/* determine the port base address associated with the pin identifier */

	if (pin_id < K64_PIN_PTB0) {		   /* Port A pin */

		return config_port_a(port_addr_ptr);

	} else if (pin_id < K64_PIN_PTC0) {	/* Port B pin */

		return config_port_b(port_addr_ptr);

	} else if (pin_id < K64_PIN_PTD0) {	/* Port C pin */

		return config_port_c(port_addr_ptr);

	} else if (pin_id < K64_PIN_PTE0) {	/* Port D pin */

		return config_port_d(port_addr_ptr);

	} else {						/* Port E pin */

		return config_port_e(port_addr_ptr);

	}

}


static int _fsl_k64_get_gpio_dev(struct device *dev,
								   mem_addr_t port_base_addr,
								   struct device **gpio_dev_ptr)
{
	struct fsl_k64_data * const data = dev->driver_data;

	/* determine the gpio device associated with the port base address */

	switch (port_base_addr) {
	case CONFIG_PORT_K64_A_BASE_ADDR:
		*gpio_dev_ptr = data->gpio_a;
		break;
	case CONFIG_PORT_K64_B_BASE_ADDR:
		*gpio_dev_ptr = data->gpio_b;
		break;
	case CONFIG_PORT_K64_C_BASE_ADDR:
		*gpio_dev_ptr = data->gpio_c;
		break;
	case CONFIG_PORT_K64_D_BASE_ADDR:
		*gpio_dev_ptr = data->gpio_d;
		break;
	case CONFIG_PORT_K64_E_BASE_ADDR:
		*gpio_dev_ptr = data->gpio_e;
		break;
	default:
		return DEV_NO_ACCESS;
	}

	return DEV_OK;
}


static uint32_t _fsl_k64_set_pin(struct device *dev,
								   uint32_t pin_id,
								   uint32_t func)
{
	mem_addr_t port_base_addr;
	uint8_t port_pin;
	uint32_t status;
	struct device *gpio_dev = NULL;
	bool is_gpio = false;
	int gpio_setting;

	if (pin_id >= CONFIG_PINMUX_NUM_PINS) {

		return DEV_INVALID_OP;
	}

	if ((func & K64_PINMUX_ALT_MASK) == K64_PINMUX_FUNC_GPIO) {
		is_gpio = true;
	}

	/* determine the pin's port register base address */

	status = _fsl_k64_get_port_addr(pin_id, &port_base_addr);

	if (status != DEV_OK) {
		return status;
	}

	/* extract the pin number within its port */

	port_pin = PIN_FROM_ID(pin_id);

	if (is_gpio) {

		/* set GPIO direction */

		status = _fsl_k64_get_gpio_dev(dev, port_base_addr, &gpio_dev);

		if (status != DEV_OK) {
			return status;
		} else if (gpio_dev == NULL) {
			return DEV_NOT_CONFIG;
		}

		if (func & K64_PINMUX_GPIO_DIR_OUTPUT) {
			gpio_setting = GPIO_DIR_OUT;
		} else {
			gpio_setting = GPIO_DIR_IN;
		}

		status = gpio_pin_configure(gpio_dev, port_pin, gpio_setting);

		if (status != DEV_OK) {
			return status;
		}

		/* remove GPIO direction info from the pin configuration */

		func &= ~K64_PINMUX_GPIO_DIR_MASK;
	}

	/* set pin function and control settings */

	sys_write32(func, port_base_addr + K64_PINMUX_CTRL_OFFSET(port_pin));

	return DEV_OK;
}

static uint32_t _fsl_k64_get_pin(struct device *dev,
								   uint32_t pin_id,
								   uint32_t *func)
{
	mem_addr_t port_base_addr;
	uint8_t port_pin;
	struct device *gpio_dev = NULL;
	const struct gpio_k64_config *cfg;
	uint32_t gpio_port_dir;
	uint32_t status;

	if (pin_id >= CONFIG_PINMUX_NUM_PINS) {
		return DEV_INVALID_OP;
	}

	/* determine the pin's port register base address */

	status = _fsl_k64_get_port_addr(pin_id, &port_base_addr);

	if (status != DEV_OK) {
		return status;
	}

	/* extract the pin number within its port */

	port_pin = PIN_FROM_ID(pin_id);

	/* get pin function and control settings */

	*func = sys_read32(port_base_addr + K64_PINMUX_CTRL_OFFSET(port_pin));

	/* get pin direction, if GPIO */

	if ((*func & K64_PINMUX_ALT_MASK) == K64_PINMUX_FUNC_GPIO) {

		status = _fsl_k64_get_gpio_dev(dev, port_base_addr, &gpio_dev);

		if (status != DEV_OK) {
			return status;
		} else if (gpio_dev == NULL) {
			return DEV_NOT_CONFIG;
		}

		cfg = gpio_dev->config->config_info;

		gpio_port_dir = sys_read32(cfg->gpio_base_addr + GPIO_K64_DIR_OFFSET);

		if (gpio_port_dir & (1 << port_pin)) {
			*func |= K64_PINMUX_GPIO_DIR_OUTPUT;
		}
	}

	return DEV_OK;
}

static uint32_t fsl_k64_dev_set(struct device *dev,
								  uint32_t pin,
								  uint32_t func)
{
	return _fsl_k64_set_pin(dev, pin, func);
}

static uint32_t fsl_k64_dev_get(struct device *dev,
								  uint32_t pin,
								  uint32_t *func)
{
	return _fsl_k64_get_pin(dev, pin, func);
}

static struct pinmux_driver_api api_funcs = {
	.set = fsl_k64_dev_set,
	.get = fsl_k64_dev_get
};

int pinmux_fsl_k64_initialize(struct device *port)
{
	struct fsl_k64_data *data = port->driver_data;

	port->driver_api = &api_funcs;

	/* get the GPIO ports, by name */

#ifdef CONFIG_GPIO_K64_A
	data->gpio_a = device_get_binding(CONFIG_PINMUX_K64_GPIO_A_NAME);
	if (!data->gpio_a) {
		return DEV_INVALID_CONF;
	}
#endif

#ifdef CONFIG_GPIO_K64_B
	data->gpio_b = device_get_binding(CONFIG_PINMUX_K64_GPIO_B_NAME);
	if (!data->gpio_b) {
		return DEV_INVALID_CONF;
	}
#endif

#ifdef CONFIG_GPIO_K64_C
	data->gpio_c = device_get_binding(CONFIG_PINMUX_K64_GPIO_C_NAME);
	if (!data->gpio_c) {
		return DEV_INVALID_CONF;
	}
#endif

#ifdef CONFIG_GPIO_K64_D
	data->gpio_d = device_get_binding(CONFIG_PINMUX_K64_GPIO_D_NAME);
	if (!data->gpio_d) {
		return DEV_INVALID_CONF;
	}
#endif

#ifdef CONFIG_GPIO_K64_E
	data->gpio_e = device_get_binding(CONFIG_PINMUX_K64_GPIO_E_NAME);
	if (!data->gpio_e) {
		return DEV_INVALID_CONF;
	}
#endif

	return DEV_OK;
}


struct pinmux_config fsl_k64_pmux = {
	.base_address = 0x00000000,
};

struct fsl_k64_data fsl_k64_pinmux_driver = {
	.gpio_a = NULL,
	.gpio_b = NULL,
	.gpio_c = NULL,
	.gpio_d = NULL,
	.gpio_e = NULL
};

/* must be initialized after GPIO */
DEVICE_INIT(pmux, PINMUX_NAME, &pinmux_fsl_k64_initialize,
			&fsl_k64_pinmux_driver, &fsl_k64_pmux,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
