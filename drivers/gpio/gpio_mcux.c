/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_port.h>

#include "gpio_utils.h"

struct gpio_mcux_config {
	GPIO_Type *gpio_base;
	PORT_Type *port_base;
};

struct gpio_mcux_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	uint32_t pin_callback_enables;
};

static int gpio_mcux_configure(struct device *dev,
			       int access_op, uint32_t pin, int flags)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;
	PORT_Type *port_base = config->port_base;
	port_interrupt_t port_interrupt = 0;
	uint32_t mask = 0;
	uint32_t pcr = 0;
	uint8_t i;

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -ENOTSUP;
	}

	/* The flags contain options that require touching registers in the
	 * GPIO module and the corresponding PORT module.
	 *
	 * Start with the GPIO module and set up the pin direction register.
	 * 0 - pin is input, 1 - pin is output
	 */

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			gpio_base->PDDR &= ~BIT(pin);
		} else {  /* GPIO_DIR_OUT */
			gpio_base->PDDR |= BIT(pin);
		}
	} else {	/* GPIO_ACCESS_BY_PORT */
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			gpio_base->PDDR = 0x0;
		} else {  /* GPIO_DIR_OUT */
			gpio_base->PDDR = 0xFFFFFFFF;
		}
	}

	/* Now do the PORT module. Figure out the pullup/pulldown
	 * configuration, but don't write it to the PCR register yet.
	 */
	mask |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_UP) {
		/* Enable the pull and select the pullup resistor. */
		pcr |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	} else if ((flags & GPIO_PUD_MASK) == GPIO_PUD_PULL_DOWN) {
		/* Enable the pull and select the pulldown resistor (deselect
		 * the pullup resistor.
		 */
		pcr |= PORT_PCR_PE_MASK;
	}

	/* Still in the PORT module. Figure out the interrupt configuration,
	 * but don't write it to the PCR register yet.
	 */
	mask |= PORT_PCR_IRQC_MASK;

	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				port_interrupt = kPORT_InterruptRisingEdge;
			} else if (flags & GPIO_INT_DOUBLE_EDGE) {
				port_interrupt = kPORT_InterruptEitherEdge;
			} else {
				port_interrupt = kPORT_InterruptFallingEdge;
			}
		} else { /* GPIO_INT_LEVEL */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				port_interrupt = kPORT_InterruptLogicOne;
			} else {
				port_interrupt = kPORT_InterruptLogicZero;
			}
		}
		pcr |= PORT_PCR_IRQC(port_interrupt);
	}

	/* Now we can write the PORT PCR register(s). If accessing by pin, we
	 * only need to write one PCR register. Otherwise, write all the PCR
	 * registers in the PORT module (one for each pin).
	 */
	if (access_op == GPIO_ACCESS_BY_PIN) {
		port_base->PCR[pin] = (port_base->PCR[pin] & ~mask) | pcr;
	} else {  /* GPIO_ACCESS_BY_PORT */
		for (i = 0; i < ARRAY_SIZE(port_base->PCR); i++) {
			port_base->PCR[i] = (port_base->PCR[pin] & ~mask) | pcr;
		}
	}

	return 0;
}

static int gpio_mcux_write(struct device *dev,
			   int access_op, uint32_t pin, uint32_t value)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			/* Set the data output for the corresponding pin.
			 * Writing zeros to the other bits leaves the data
			 * output unchanged for the other pins.
			 */
			gpio_base->PSOR = BIT(pin);
		} else {
			/* Clear the data output for the corresponding pin.
			 * Writing zeros to the other bits leaves the data
			 * output unchanged for the other pins.
			 */
			gpio_base->PCOR = BIT(pin);
		}
	} else { /* GPIO_ACCESS_BY_PORT */
		/* Write the data output for all the pins */
		gpio_base->PDOR = value;
	}

	return 0;
}

static int gpio_mcux_read(struct device *dev,
			  int access_op, uint32_t pin, uint32_t *value)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PDIR;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (*value & BIT(pin)) >> pin;
	}

	/* nothing more to do for GPIO_ACCESS_BY_PORT */

	return 0;
}

static int gpio_mcux_manage_callback(struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct gpio_mcux_data *data = dev->driver_data;

	_gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int gpio_mcux_enable_callback(struct device *dev,
				     int access_op, uint32_t pin)
{
	struct gpio_mcux_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= BIT(pin);
	} else {
		data->pin_callback_enables = 0xFFFFFFFF;
	}

	return 0;
}

static int gpio_mcux_disable_callback(struct device *dev,
				      int access_op, uint32_t pin)
{
	struct gpio_mcux_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables &= ~BIT(pin);
	} else {
		data->pin_callback_enables = 0;
	}

	return 0;
}

static void gpio_mcux_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_mcux_config *config = dev->config->config_info;
	struct gpio_mcux_data *data = dev->driver_data;
	uint32_t enabled_int, int_status;

	int_status = config->port_base->ISFR;
	enabled_int = int_status & data->pin_callback_enables;

	_gpio_fire_callbacks(&data->callbacks, dev, enabled_int);

	/* Clear the port interrupts */
	config->port_base->ISFR = 0xFFFFFFFF;
}


static const struct gpio_driver_api gpio_mcux_driver_api = {
	.config = gpio_mcux_configure,
	.write = gpio_mcux_write,
	.read = gpio_mcux_read,
	.manage_callback = gpio_mcux_manage_callback,
	.enable_callback = gpio_mcux_enable_callback,
	.disable_callback = gpio_mcux_disable_callback,
};

#ifdef CONFIG_GPIO_MCUX_PORTA
static int gpio_mcux_porta_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_porta_config = {
	.gpio_base = GPIOA,
	.port_base = PORTA,
};

static struct gpio_mcux_data gpio_mcux_porta_data;

DEVICE_AND_API_INIT(gpio_mcux_porta, CONFIG_GPIO_MCUX_PORTA_NAME,
		    gpio_mcux_porta_init,
		    &gpio_mcux_porta_data, &gpio_mcux_porta_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_porta_init(struct device *dev)
{
	IRQ_CONNECT(IRQ_GPIO_PORTA, CONFIG_GPIO_MCUX_PORTA_PRI,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_porta), 0);

	irq_enable(IRQ_GPIO_PORTA);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTA */

#ifdef CONFIG_GPIO_MCUX_PORTB
static int gpio_mcux_portb_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_portb_config = {
	.gpio_base = GPIOB,
	.port_base = PORTB,
};

static struct gpio_mcux_data gpio_mcux_portb_data;

DEVICE_AND_API_INIT(gpio_mcux_portb, CONFIG_GPIO_MCUX_PORTB_NAME,
		    gpio_mcux_portb_init,
		    &gpio_mcux_portb_data, &gpio_mcux_portb_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_portb_init(struct device *dev)
{
	IRQ_CONNECT(IRQ_GPIO_PORTB, CONFIG_GPIO_MCUX_PORTB_PRI,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_portb), 0);

	irq_enable(IRQ_GPIO_PORTB);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTB */

#ifdef CONFIG_GPIO_MCUX_PORTC
static int gpio_mcux_portc_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_portc_config = {
	.gpio_base = GPIOC,
	.port_base = PORTC,
};

static struct gpio_mcux_data gpio_mcux_portc_data;

DEVICE_AND_API_INIT(gpio_mcux_portc, CONFIG_GPIO_MCUX_PORTC_NAME,
		    gpio_mcux_portc_init,
		    &gpio_mcux_portc_data, &gpio_mcux_portc_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_portc_init(struct device *dev)
{
	IRQ_CONNECT(IRQ_GPIO_PORTC, CONFIG_GPIO_MCUX_PORTC_PRI,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_portc), 0);

	irq_enable(IRQ_GPIO_PORTC);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTC */

#ifdef CONFIG_GPIO_MCUX_PORTD
static int gpio_mcux_portd_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_portd_config = {
	.gpio_base = GPIOD,
	.port_base = PORTD,
};

static struct gpio_mcux_data gpio_mcux_portd_data;

DEVICE_AND_API_INIT(gpio_mcux_portd, CONFIG_GPIO_MCUX_PORTD_NAME,
		    gpio_mcux_portd_init,
		    &gpio_mcux_portd_data, &gpio_mcux_portd_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_portd_init(struct device *dev)
{
	IRQ_CONNECT(IRQ_GPIO_PORTD, CONFIG_GPIO_MCUX_PORTD_PRI,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_portd), 0);

	irq_enable(IRQ_GPIO_PORTD);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTD */

#ifdef CONFIG_GPIO_MCUX_PORTE
static int gpio_mcux_porte_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_porte_config = {
	.gpio_base = GPIOE,
	.port_base = PORTE,
};

static struct gpio_mcux_data gpio_mcux_porte_data;

DEVICE_AND_API_INIT(gpio_mcux_porte, CONFIG_GPIO_MCUX_PORTE_NAME,
		    gpio_mcux_porte_init,
		    &gpio_mcux_porte_data, &gpio_mcux_porte_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_porte_init(struct device *dev)
{
	IRQ_CONNECT(IRQ_GPIO_PORTE, CONFIG_GPIO_MCUX_PORTE_PRI,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_porte), 0);

	irq_enable(IRQ_GPIO_PORTE);

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTE */
