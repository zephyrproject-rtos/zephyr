/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2017, NXP
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_port.h>
#include <zephyr/drivers/clock_control.h>

#include "gpio_utils.h"

struct gpio_rv32m1_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *gpio_base;
	PORT_Type *port_base;
	unsigned int flags;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	int (*irq_config_func)(const struct device *dev);
};

struct gpio_rv32m1_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

static uint32_t get_port_pcr_irqc_value_from_flags(const struct device *dev,
						   uint32_t pin,
						   enum gpio_int_mode mode,
						   enum gpio_int_trig trig)
{
	port_interrupt_t port_interrupt = 0;

	if (mode == GPIO_INT_MODE_DISABLED) {
		port_interrupt = kPORT_InterruptOrDMADisabled;
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig == GPIO_INT_TRIG_LOW) {
				port_interrupt = kPORT_InterruptLogicZero;
			} else {
				port_interrupt = kPORT_InterruptLogicOne;
			}
		} else {
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				port_interrupt = kPORT_InterruptFallingEdge;
				break;
			case GPIO_INT_TRIG_HIGH:
				port_interrupt = kPORT_InterruptRisingEdge;
				break;
			case GPIO_INT_TRIG_BOTH:
				port_interrupt = kPORT_InterruptEitherEdge;
				break;
			}
		}
	}

	return PORT_PCR_IRQC(port_interrupt);
}

static int gpio_rv32m1_configure(const struct device *dev,
				 gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rv32m1_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	PORT_Type *port_base = config->port_base;
	uint32_t mask = 0U;
	uint32_t pcr = 0U;

	/* Check for an invalid pin number */
	if (pin >= ARRAY_SIZE(port_base->PCR)) {
		return -EINVAL;
	}

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT_ENABLE) && ((flags & GPIO_INPUT) == 0)) {
		return -EINVAL;
	}

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* Check if GPIO port supports interrupts */
	if ((flags & GPIO_INT_ENABLE) &&
	    ((config->flags & GPIO_INT_ENABLE) == 0U)) {
		return -ENOTSUP;
	}

	/* The flags contain options that require touching registers in the
	 * GPIO module and the corresponding PORT module.
	 *
	 * Start with the GPIO module and set up the pin direction register.
	 * 0 - pin is input, 1 - pin is output
	 */

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		gpio_base->PDDR &= ~BIT(pin);
		break;
	case GPIO_OUTPUT:
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			gpio_base->PSOR = BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			gpio_base->PCOR = BIT(pin);
		}
		gpio_base->PDDR |= BIT(pin);
		break;
	default:
		return -ENOTSUP;
	}

	/* Set PCR mux to GPIO for the pin we are configuring */
	mask |= PORT_PCR_MUX_MASK;
	pcr |= PORT_PCR_MUX(kPORT_MuxAsGpio);

	/* Now do the PORT module. Figure out the pullup/pulldown
	 * configuration, but don't write it to the PCR register yet.
	 */
	mask |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	if ((flags & GPIO_PULL_UP) != 0) {
		/* Enable the pull and select the pullup resistor. */
		pcr |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		/* Enable the pull and select the pulldown resistor (deselect
		 * the pullup resistor.
		 */
		pcr |= PORT_PCR_PE_MASK;
	}

	/* Still in the PORT module. Figure out the interrupt configuration,
	 * but don't write it to the PCR register yet.
	 */
	mask |= PORT_PCR_IRQC_MASK;

	/* Accessing by pin, we only need to write one PCR register. */
	port_base->PCR[pin] = (port_base->PCR[pin] & ~mask) | pcr;

	return 0;
}

static int gpio_rv32m1_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_rv32m1_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PDIR;

	return 0;
}

static int gpio_rv32m1_port_set_masked_raw(const struct device *dev,
					   uint32_t mask,
					   uint32_t value)
{
	const struct gpio_rv32m1_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PDOR = (gpio_base->PDOR & ~mask) | (mask & value);

	return 0;
}

static int gpio_rv32m1_port_set_bits_raw(const struct device *dev,
					 uint32_t mask)
{
	const struct gpio_rv32m1_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PSOR = mask;

	return 0;
}

static int gpio_rv32m1_port_clear_bits_raw(const struct device *dev,
					   uint32_t mask)
{
	const struct gpio_rv32m1_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PCOR = mask;

	return 0;
}

static int gpio_rv32m1_port_toggle_bits(const struct device *dev,
					uint32_t mask)
{
	const struct gpio_rv32m1_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PTOR = mask;

	return 0;
}

static int gpio_rv32m1_pin_interrupt_configure(const struct device *dev,
					       gpio_pin_t pin,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	const struct gpio_rv32m1_config *config = dev->config;
	PORT_Type *port_base = config->port_base;

	/* Check for an invalid pin number */
	if (pin >= ARRAY_SIZE(port_base->PCR)) {
		return -EINVAL;
	}

	/* Check if GPIO port supports interrupts */
	if ((mode != GPIO_INT_MODE_DISABLED) &&
	    ((config->flags & GPIO_INT_ENABLE) == 0U)) {
		return -ENOTSUP;
	}

	uint32_t pcr = get_port_pcr_irqc_value_from_flags(dev, pin, mode, trig);

	port_base->PCR[pin] = (port_base->PCR[pin] & ~PORT_PCR_IRQC_MASK) | pcr;

	return 0;
}


static int gpio_rv32m1_manage_callback(const struct device *dev,
				       struct gpio_callback *callback,
				       bool set)
{
	struct gpio_rv32m1_data *data = dev->data;

	gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static void gpio_rv32m1_port_isr(const struct device *dev)
{
	const struct gpio_rv32m1_config *config = dev->config;
	struct gpio_rv32m1_data *data = dev->data;
	uint32_t int_status;

	int_status = config->port_base->ISFR;

	/* Clear the port interrupts before invoking callbacks */
	config->port_base->ISFR = int_status;

	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

static int gpio_rv32m1_init(const struct device *dev)
{
	const struct gpio_rv32m1_config *config = dev->config;
	int ret;

	if (config->clock_dev) {
		ret = clock_control_on(config->clock_dev, config->clock_subsys);

		if (ret < 0) {
			return ret;
		}
	}

	return config->irq_config_func(dev);
}

static const struct gpio_driver_api gpio_rv32m1_driver_api = {
	.pin_configure = gpio_rv32m1_configure,
	.port_get_raw = gpio_rv32m1_port_get_raw,
	.port_set_masked_raw = gpio_rv32m1_port_set_masked_raw,
	.port_set_bits_raw = gpio_rv32m1_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rv32m1_port_clear_bits_raw,
	.port_toggle_bits = gpio_rv32m1_port_toggle_bits,
	.pin_interrupt_configure = gpio_rv32m1_pin_interrupt_configure,
	.manage_callback = gpio_rv32m1_manage_callback,
};

#define INST_DT_PORT_ADDR(n) \
	DT_REG_ADDR(DT_INST_PHANDLE(n, openisa_rv32m1_port))
#define INST_DT_CLK_CTRL_DEV(n) \
	UTIL_AND(DT_INST_NODE_HAS_PROP(n, clocks), DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)))
#define INST_DT_CLK_CELL_NAME(n) \
	UTIL_AND(DT_INST_NODE_HAS_PROP(n, clocks), DT_INST_CLOCKS_CELL(n, name))

#define GPIO_RV32M1_INIT(n) \
	static int gpio_rv32m1_##n##_init(const struct device *dev);	\
									\
	static const struct gpio_rv32m1_config gpio_rv32m1_##n##_config = {\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.gpio_base = (GPIO_Type *) DT_INST_REG_ADDR(n),		\
		.port_base = (PORT_Type *) INST_DT_PORT_ADDR(n),	\
		.flags = GPIO_INT_ENABLE,				\
		.irq_config_func = gpio_rv32m1_##n##_init,		\
		.clock_dev = INST_DT_CLK_CTRL_DEV(n),			\
		.clock_subsys = (clock_control_subsys_t)		\
				INST_DT_CLK_CELL_NAME(n)		\
	};								\
									\
	static struct gpio_rv32m1_data gpio_rv32m1_##n##_data;		\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    gpio_rv32m1_init,				\
			    NULL,					\
			    &gpio_rv32m1_##n##_data,			\
			    &gpio_rv32m1_##n##_config,			\
			    PRE_KERNEL_1,				\
			    CONFIG_GPIO_INIT_PRIORITY,			\
			    &gpio_rv32m1_driver_api);			\
									\
	static int gpio_rv32m1_##n##_init(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    0,						\
			    gpio_rv32m1_port_isr,			\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(0));				\
									\
		return 0;						\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_RV32M1_INIT)
