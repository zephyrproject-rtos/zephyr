/*
 * Copyright (c) 2017, Christian Taedcke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <gpio.h>
#include <soc.h>
#include <em_gpio.h>

#include "gpio_utils.h"

/*
 * Macros to set the GPIO MODE registers
 *
 * See https://www.silabs.com/documents/public/reference-manuals/EFM32WG-RM.pdf
 * pages 972 and 982.
 */
/**
 * @brief Create the value to set the GPIO MODEL register
 * @param[in] pin The index of the pin. Valid values are 0..7.
 * @param[in] mode The mode that should be set.
 * @return The value that can be set into the GPIO MODEL register.
 */
#define GECKO_GPIO_MODEL(pin, mode) (mode << (pin * 4))

/**
 * @brief Create the value to set the GPIO MODEH register
 * @param[in] pin The index of the pin. Valid values are 8..15.
 * @param[in] mode The mode that should be set.
 * @return The value that can be set into the GPIO MODEH register.
 */
#define GECKO_GPIO_MODEH(pin, mode) (mode << ((pin - 8) * 4))


#define member_size(type, member) sizeof(((type *)0)->member)
#define NUMBER_OF_PORTS (member_size(GPIO_TypeDef, P) / \
			 member_size(GPIO_TypeDef, P[0]))

struct gpio_gecko_common_config {
};

struct gpio_gecko_common_data {
	/* a list of all ports */
	struct device *ports[NUMBER_OF_PORTS];
	size_t count;
};

struct gpio_gecko_config {
	GPIO_P_TypeDef *gpio_base;
	GPIO_Port_TypeDef gpio_index;
};

struct gpio_gecko_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

static inline void gpio_gecko_add_port(struct gpio_gecko_common_data *data,
				       struct device *dev)
{
	__ASSERT(dev, "No port device!");
	data->ports[data->count++] = dev;
}


static int gpio_gecko_configure(struct device *dev,
				int access_op, u32_t pin, int flags)
{
	const struct gpio_gecko_config *config = dev->config->config_info;
	GPIO_P_TypeDef *gpio_base = config->gpio_base;
	GPIO_Port_TypeDef gpio_index = config->gpio_index;
	GPIO_Mode_TypeDef mode;
	unsigned int out = 0;

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	/* Interrupt on static level is not supported by the hardware */
	if ((flags & GPIO_INT) && !(flags & GPIO_INT_EDGE)) {
		return -ENOTSUP;
	}

	/* Setting interrupt flags for a complete port is not implemented */
	if ((flags & GPIO_INT) && (access_op == GPIO_ACCESS_BY_PORT)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
		if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
			mode = gpioModeInputPull;
			out = 1; /* pull-up*/
		} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
			mode = gpioModeInputPull;
			/* out = 0 means pull-down*/
		} else {
			mode = gpioModeInput;
		}
	} else { /* GPIO_DIR_OUT */
		mode = gpioModePushPull;
	}
	/* The flags contain options that require touching registers in the
	 * GPIO module and the corresponding PORT module.
	 *
	 * Start with the GPIO module and set up the pin direction register.
	 * 0 - pin is input, 1 - pin is output
	 */

	if (access_op == GPIO_ACCESS_BY_PIN) {
		GPIO_PinModeSet(gpio_index, pin, mode, out);
	} else {	/* GPIO_ACCESS_BY_PORT */
		gpio_base->MODEL = GECKO_GPIO_MODEL(7, mode)
			| GECKO_GPIO_MODEL(6, mode) | GECKO_GPIO_MODEL(5, mode)
			| GECKO_GPIO_MODEL(4, mode) | GECKO_GPIO_MODEL(3, mode)
			| GECKO_GPIO_MODEL(2, mode) | GECKO_GPIO_MODEL(1, mode)
			| GECKO_GPIO_MODEL(0, mode);
		gpio_base->MODEH = GECKO_GPIO_MODEH(15, mode)
			| GECKO_GPIO_MODEH(14, mode)
			| GECKO_GPIO_MODEH(13, mode)
			| GECKO_GPIO_MODEH(12, mode)
			| GECKO_GPIO_MODEH(11, mode)
			| GECKO_GPIO_MODEH(10, mode)
			| GECKO_GPIO_MODEH(9, mode)
			| GECKO_GPIO_MODEH(8, mode);
		gpio_base->DOUT = (out ? 0xFFFF : 0x0000);
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		GPIO_IntConfig(gpio_index, pin,
			       (flags & GPIO_INT_ACTIVE_HIGH)
			       || (flags & GPIO_INT_DOUBLE_EDGE),
			       !(flags & GPIO_INT_ACTIVE_HIGH)
			       || (flags & GPIO_INT_DOUBLE_EDGE),
			       !!(flags & GPIO_INT));
	}

	return 0;
}

static int gpio_gecko_write(struct device *dev,
			    int access_op, u32_t pin, u32_t value)
{
	const struct gpio_gecko_config *config = dev->config->config_info;
	GPIO_P_TypeDef *gpio_base = config->gpio_base;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			/* Set the data output for the corresponding pin.
			 * Writing zeros to the other bits leaves the data
			 * output unchanged for the other pins.
			 */
			gpio_base->DOUTSET = BIT(pin);
		} else {
			/* Clear the data output for the corresponding pin.
			 * Writing zeros to the other bits leaves the data
			 * output unchanged for the other pins.
			 */
			gpio_base->DOUTCLR = BIT(pin);
		}
	} else { /* GPIO_ACCESS_BY_PORT */
		/* Write the data output for all the pins */
		gpio_base->DOUT = value;
	}

	return 0;
}

static int gpio_gecko_read(struct device *dev,
			   int access_op, u32_t pin, u32_t *value)
{
	const struct gpio_gecko_config *config = dev->config->config_info;
	GPIO_P_TypeDef *gpio_base = config->gpio_base;

	*value = gpio_base->DIN;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (*value & BIT(pin)) >> pin;
	}

	/* nothing more to do for GPIO_ACCESS_BY_PORT */

	return 0;
}

static int gpio_gecko_manage_callback(struct device *dev,
				      struct gpio_callback *callback, bool set)
{
	struct gpio_gecko_data *data = dev->driver_data;

	_gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int gpio_gecko_enable_callback(struct device *dev,
				      int access_op, u32_t pin)
{
	struct gpio_gecko_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PORT) {
		return -ENOTSUP;
	}

	data->pin_callback_enables |= BIT(pin);
	GPIO->IEN |= BIT(pin);

	return 0;
}

static int gpio_gecko_disable_callback(struct device *dev,
				       int access_op, u32_t pin)
{
	struct gpio_gecko_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PORT) {
		return -ENOTSUP;
	}

	data->pin_callback_enables &= ~BIT(pin);
	GPIO->IEN &= ~BIT(pin);

	return 0;
}

/**
 * Handler for both odd and even pin interrupts
 */
static void gpio_gecko_common_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct gpio_gecko_common_data *data = dev->driver_data;
	u32_t enabled_int, int_status;
	struct device *port_dev;
	struct gpio_gecko_data *port_data;

	int_status = GPIO->IF;

	for (unsigned int i = 0; int_status && (i < data->count); i++) {
		port_dev = data->ports[i];
		port_data = port_dev->driver_data;
		enabled_int = int_status & port_data->pin_callback_enables;
		int_status &= ~enabled_int;

		_gpio_fire_callbacks(&port_data->callbacks, port_dev,
				     enabled_int);
	}
	/* Clear the pending interrupts */
	GPIO->IFC = 0xFFFF;
}


static const struct gpio_driver_api gpio_gecko_driver_api = {
	.config = gpio_gecko_configure,
	.write = gpio_gecko_write,
	.read = gpio_gecko_read,
	.manage_callback = gpio_gecko_manage_callback,
	.enable_callback = gpio_gecko_enable_callback,
	.disable_callback = gpio_gecko_disable_callback,
};

static const struct gpio_driver_api gpio_gecko_common_driver_api = {
	.manage_callback = gpio_gecko_manage_callback,
	.enable_callback = gpio_gecko_enable_callback,
	.disable_callback = gpio_gecko_disable_callback,
};

#ifdef CONFIG_GPIO_GECKO
static int gpio_gecko_common_init(struct device *dev);

static const struct gpio_gecko_common_config gpio_gecko_common_config = {
};

static struct gpio_gecko_common_data gpio_gecko_common_data;

DEVICE_AND_API_INIT(gpio_gecko_common, CONFIG_GPIO_GECKO_COMMON_NAME,
		    gpio_gecko_common_init,
		    &gpio_gecko_common_data, &gpio_gecko_common_config,
		    POST_KERNEL, CONFIG_GPIO_GECKO_COMMON_INIT_PRIORITY,
		    &gpio_gecko_common_driver_api);

static int gpio_gecko_common_init(struct device *dev)
{
	gpio_gecko_common_data.count = 0;
	IRQ_CONNECT(GPIO_EVEN_IRQn, CONFIG_GPIO_GECKO_COMMON_PRI,
		    gpio_gecko_common_isr, DEVICE_GET(gpio_gecko_common), 0);

	IRQ_CONNECT(GPIO_ODD_IRQn, CONFIG_GPIO_GECKO_COMMON_PRI,
		    gpio_gecko_common_isr, DEVICE_GET(gpio_gecko_common), 0);

	irq_enable(GPIO_EVEN_IRQn);
	irq_enable(GPIO_ODD_IRQn);

	return 0;
}
#endif /* CONFIG_GPIO_GECKO */


#ifdef CONFIG_GPIO_GECKO_PORTA
static int gpio_gecko_porta_init(struct device *dev);

static const struct gpio_gecko_config gpio_gecko_porta_config = {
	.gpio_base = &GPIO->P[gpioPortA],
	.gpio_index = gpioPortA,
};

static struct gpio_gecko_data gpio_gecko_porta_data;

DEVICE_AND_API_INIT(gpio_gecko_porta, CONFIG_GPIO_GECKO_PORTA_NAME,
		    gpio_gecko_porta_init,
		    &gpio_gecko_porta_data, &gpio_gecko_porta_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_gecko_driver_api);

static int gpio_gecko_porta_init(struct device *dev)
{
	gpio_gecko_add_port(&gpio_gecko_common_data, dev);
	return 0;
}
#endif /* CONFIG_GPIO_GECKO_PORTA */

#ifdef CONFIG_GPIO_GECKO_PORTB
static int gpio_gecko_portb_init(struct device *dev);

static const struct gpio_gecko_config gpio_gecko_portb_config = {
	.gpio_base = &GPIO->P[gpioPortB],
	.gpio_index = gpioPortB,
};

static struct gpio_gecko_data gpio_gecko_portb_data;

DEVICE_AND_API_INIT(gpio_gecko_portb, CONFIG_GPIO_GECKO_PORTB_NAME,
		    gpio_gecko_portb_init,
		    &gpio_gecko_portb_data, &gpio_gecko_portb_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_gecko_driver_api);

static int gpio_gecko_portb_init(struct device *dev)
{
	gpio_gecko_add_port(&gpio_gecko_common_data, dev);
	return 0;
}
#endif /* CONFIG_GPIO_GECKO_PORTB */

#ifdef CONFIG_GPIO_GECKO_PORTC
static int gpio_gecko_portc_init(struct device *dev);

static const struct gpio_gecko_config gpio_gecko_portc_config = {
	.gpio_base = &GPIO->P[gpioPortC],
	.gpio_index = gpioPortC,
};

static struct gpio_gecko_data gpio_gecko_portc_data;

DEVICE_AND_API_INIT(gpio_gecko_portc, CONFIG_GPIO_GECKO_PORTC_NAME,
		    gpio_gecko_portc_init,
		    &gpio_gecko_portc_data, &gpio_gecko_portc_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_gecko_driver_api);

static int gpio_gecko_portc_init(struct device *dev)
{
	gpio_gecko_add_port(&gpio_gecko_common_data, dev);
	return 0;
}
#endif /* CONFIG_GPIO_GECKO_PORTC */

#ifdef CONFIG_GPIO_GECKO_PORTD
static int gpio_gecko_portd_init(struct device *dev);

static const struct gpio_gecko_config gpio_gecko_portd_config = {
	.gpio_base = &GPIO->P[gpioPortD],
	.gpio_index = gpioPortD,
};

static struct gpio_gecko_data gpio_gecko_portd_data;

DEVICE_AND_API_INIT(gpio_gecko_portd, CONFIG_GPIO_GECKO_PORTD_NAME,
		    gpio_gecko_portd_init,
		    &gpio_gecko_portd_data, &gpio_gecko_portd_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_gecko_driver_api);

static int gpio_gecko_portd_init(struct device *dev)
{
	gpio_gecko_add_port(&gpio_gecko_common_data, dev);
	return 0;
}
#endif /* CONFIG_GPIO_GECKO_PORTD */

#ifdef CONFIG_GPIO_GECKO_PORTE
static int gpio_gecko_porte_init(struct device *dev);

static const struct gpio_gecko_config gpio_gecko_porte_config = {
	.gpio_base = &GPIO->P[gpioPortE],
	.gpio_index = gpioPortE,
};

static struct gpio_gecko_data gpio_gecko_porte_data;

DEVICE_AND_API_INIT(gpio_gecko_porte, CONFIG_GPIO_GECKO_PORTE_NAME,
		    gpio_gecko_porte_init,
		    &gpio_gecko_porte_data, &gpio_gecko_porte_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_gecko_driver_api);

static int gpio_gecko_porte_init(struct device *dev)
{
	gpio_gecko_add_port(&gpio_gecko_common_data, dev);
	return 0;
}
#endif /* CONFIG_GPIO_GECKO_PORTE */

#ifdef CONFIG_GPIO_GECKO_PORTF
static int gpio_gecko_portf_init(struct device *dev);

static const struct gpio_gecko_config gpio_gecko_portf_config = {
	.gpio_base = &GPIO->P[gpioPortF],
	.gpio_index = gpioPortF,
};

static struct gpio_gecko_data gpio_gecko_portf_data;

DEVICE_AND_API_INIT(gpio_gecko_portf, CONFIG_GPIO_GECKO_PORTF_NAME,
		    gpio_gecko_portf_init,
		    &gpio_gecko_portf_data, &gpio_gecko_portf_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_gecko_driver_api);

static int gpio_gecko_portf_init(struct device *dev)
{
	gpio_gecko_add_port(&gpio_gecko_common_data, dev);
	return 0;
}
#endif /* CONFIG_GPIO_GECKO_PORTF */
