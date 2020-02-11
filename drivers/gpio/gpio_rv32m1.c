/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2017, NXP
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_port.h>
#include <drivers/clock_control.h>

#include "gpio_utils.h"

struct gpio_rv32m1_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *gpio_base;
	PORT_Type *port_base;
	unsigned int flags;
	char *clock_controller;
	clock_control_subsys_t clock_subsys;
	int (*irq_config_func)(struct device *dev);
};

struct gpio_rv32m1_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

static u32_t get_port_pcr_irqc_value_from_flags(struct device *dev,
		u32_t pin, enum gpio_int_mode mode,
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

static int gpio_rv32m1_configure(struct device *dev,
				 gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;
	PORT_Type *port_base = config->port_base;
	struct gpio_rv32m1_data *data = dev->driver_data;
	u32_t mask = 0U;
	u32_t pcr = 0U;

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
	WRITE_BIT(data->pin_callback_enables, pin,
		  flags & GPIO_INT_ENABLE);

	return 0;
}
static int gpio_rv32m1_port_get_raw(struct device *dev, u32_t *value)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PDIR;

	return 0;
}

static int gpio_rv32m1_port_set_masked_raw(struct device *dev, u32_t mask,
		u32_t value)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PDOR = (gpio_base->PDOR & ~mask) | (mask & value);

	return 0;
}

static int gpio_rv32m1_port_set_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PSOR = mask;

	return 0;
}

static int gpio_rv32m1_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PCOR = mask;

	return 0;
}

static int gpio_rv32m1_port_toggle_bits(struct device *dev, u32_t mask)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PTOR = mask;

	return 0;
}

static int gpio_rv32m1_pin_interrupt_configure(struct device *dev,
		gpio_pin_t pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	PORT_Type *port_base = config->port_base;
	struct gpio_rv32m1_data *data = dev->driver_data;

	/* Check for an invalid pin number */
	if (pin >= ARRAY_SIZE(port_base->PCR)) {
		return -EINVAL;
	}

	/* Check if GPIO port supports interrupts */
	if ((mode != GPIO_INT_MODE_DISABLED) &&
	    ((config->flags & GPIO_INT_ENABLE) == 0U)) {
		return -ENOTSUP;
	}

	u32_t pcr = get_port_pcr_irqc_value_from_flags(dev, pin, mode, trig);

	port_base->PCR[pin] = (port_base->PCR[pin] & ~PORT_PCR_IRQC_MASK) | pcr;

	WRITE_BIT(data->pin_callback_enables, pin, mode != GPIO_INT_DISABLE);

	return 0;
}


static int gpio_rv32m1_manage_callback(struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct gpio_rv32m1_data *data = dev->driver_data;

	gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int gpio_rv32m1_enable_callback(struct device *dev,
				     gpio_pin_t pin)
{
	struct gpio_rv32m1_data *data = dev->driver_data;

	data->pin_callback_enables |= BIT(pin);

	return 0;
}

static int gpio_rv32m1_disable_callback(struct device *dev,
				      gpio_pin_t pin)
{
	struct gpio_rv32m1_data *data = dev->driver_data;

	data->pin_callback_enables &= ~BIT(pin);

	return 0;
}

static void gpio_rv32m1_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	struct gpio_rv32m1_data *data = dev->driver_data;
	u32_t enabled_int, int_status;

	int_status = config->port_base->ISFR;
	enabled_int = int_status & data->pin_callback_enables;

	/* Clear the port interrupts before invoking callbacks */
	config->port_base->ISFR = enabled_int;

	gpio_fire_callbacks(&data->callbacks, dev, enabled_int);

}

static int gpio_rv32m1_init(struct device *dev)
{
	const struct gpio_rv32m1_config *config = dev->config->config_info;
	struct device *clk;
	int ret;

	if (config->clock_controller) {
		clk = device_get_binding(config->clock_controller);
		if (!clk) {
			return -ENODEV;
		}

		ret = clock_control_on(clk, config->clock_subsys);

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
	.enable_callback = gpio_rv32m1_enable_callback,
	.disable_callback = gpio_rv32m1_disable_callback,
};

#ifdef CONFIG_GPIO_RV32M1_PORTA
static int gpio_rv32m1_porta_init(struct device *dev);

static const struct gpio_rv32m1_config gpio_rv32m1_porta_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_0_OPENISA_RV32M1_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_A_BASE_ADDRESS,
	.port_base = PORTA,
#ifdef DT_ALIAS_GPIO_A_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
	.irq_config_func = gpio_rv32m1_porta_init,
#ifdef DT_ALIAS_GPIO_A_CLOCK_CONTROLLER
	.clock_controller = DT_ALIAS_GPIO_A_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)
			DT_ALIAS_GPIO_A_CLOCK_NAME,
#else
	.clock_controller = NULL,
#endif
};

static struct gpio_rv32m1_data gpio_rv32m1_porta_data;

DEVICE_AND_API_INIT(gpio_rv32m1_porta, DT_ALIAS_GPIO_A_LABEL,
		    gpio_rv32m1_init,
		    &gpio_rv32m1_porta_data, &gpio_rv32m1_porta_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_rv32m1_driver_api);

static int gpio_rv32m1_porta_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_A_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_A_IRQ_0,
		    0,
		    gpio_rv32m1_port_isr, DEVICE_GET(gpio_rv32m1_porta), 0);

	irq_enable(DT_ALIAS_GPIO_A_IRQ_0);

	return 0;
#else
	return -1;
#endif
}
#endif /* CONFIG_GPIO_RV32M1_PORTA */

#ifdef CONFIG_GPIO_RV32M1_PORTB
static int gpio_rv32m1_portb_init(struct device *dev);

static const struct gpio_rv32m1_config gpio_rv32m1_portb_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_1_OPENISA_RV32M1_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_B_BASE_ADDRESS,
	.port_base = PORTB,
#ifdef DT_ALIAS_GPIO_B_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
	.irq_config_func = gpio_rv32m1_portb_init,
#ifdef DT_ALIAS_GPIO_B_CLOCK_CONTROLLER
	.clock_controller = DT_ALIAS_GPIO_B_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)
			DT_ALIAS_GPIO_B_CLOCK_NAME,
#else
	.clock_controller = NULL,
#endif
};

static struct gpio_rv32m1_data gpio_rv32m1_portb_data;

DEVICE_AND_API_INIT(gpio_rv32m1_portb, DT_ALIAS_GPIO_B_LABEL,
		    gpio_rv32m1_init,
		    &gpio_rv32m1_portb_data, &gpio_rv32m1_portb_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_rv32m1_driver_api);

static int gpio_rv32m1_portb_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_B_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_B_IRQ_0,
		    0,
		    gpio_rv32m1_port_isr, DEVICE_GET(gpio_rv32m1_portb), 0);

	irq_enable(DT_ALIAS_GPIO_B_IRQ_0);

	return 0;
#else
	return -1;
#endif
}
#endif /* CONFIG_GPIO_RV32M1_PORTB */

#ifdef CONFIG_GPIO_RV32M1_PORTC
static int gpio_rv32m1_portc_init(struct device *dev);

static const struct gpio_rv32m1_config gpio_rv32m1_portc_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_2_OPENISA_RV32M1_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_C_BASE_ADDRESS,
	.port_base = PORTC,
#ifdef DT_ALIAS_GPIO_C_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
	.irq_config_func = gpio_rv32m1_portc_init,
#ifdef DT_ALIAS_GPIO_C_CLOCK_CONTROLLER
	.clock_controller = DT_ALIAS_GPIO_C_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)
			DT_ALIAS_GPIO_C_CLOCK_NAME,
#else
	.clock_controller = NULL,
#endif

};

static struct gpio_rv32m1_data gpio_rv32m1_portc_data;

DEVICE_AND_API_INIT(gpio_rv32m1_portc, DT_ALIAS_GPIO_C_LABEL,
		    gpio_rv32m1_init,
		    &gpio_rv32m1_portc_data, &gpio_rv32m1_portc_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_rv32m1_driver_api);

static int gpio_rv32m1_portc_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_C_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_C_IRQ_0,
		    0,
		    gpio_rv32m1_port_isr, DEVICE_GET(gpio_rv32m1_portc), 0);

	irq_enable(DT_ALIAS_GPIO_C_IRQ_0);

	return 0;
#else
	return -1;
#endif
}
#endif /* CONFIG_GPIO_RV32M1_PORTC */

#ifdef CONFIG_GPIO_RV32M1_PORTD
static int gpio_rv32m1_portd_init(struct device *dev);

static const struct gpio_rv32m1_config gpio_rv32m1_portd_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_3_OPENISA_RV32M1_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_D_BASE_ADDRESS,
	.port_base = PORTD,
#ifdef DT_ALIAS_GPIO_D_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
	.irq_config_func = gpio_rv32m1_portd_init,
#ifdef DT_ALIAS_GPIO_D_CLOCK_CONTROLLER
	.clock_controller = DT_ALIAS_GPIO_D_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)
			DT_ALIAS_GPIO_D_CLOCK_NAME,
#else
	.clock_controller = NULL,
#endif
};

static struct gpio_rv32m1_data gpio_rv32m1_portd_data;

DEVICE_AND_API_INIT(gpio_rv32m1_portd, DT_ALIAS_GPIO_D_LABEL,
		    gpio_rv32m1_init,
		    &gpio_rv32m1_portd_data, &gpio_rv32m1_portd_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_rv32m1_driver_api);

static int gpio_rv32m1_portd_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_D_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_D_IRQ_0,
		    0,
		    gpio_rv32m1_port_isr, DEVICE_GET(gpio_rv32m1_portd), 0);

	irq_enable(DT_ALIAS_GPIO_D_IRQ_0);

	return 0;
#else
	return -1;
#endif
}
#endif /* CONFIG_GPIO_RV32M1_PORTD */

#ifdef CONFIG_GPIO_RV32M1_PORTE
static int gpio_rv32m1_porte_init(struct device *dev);

static const struct gpio_rv32m1_config gpio_rv32m1_porte_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_4_OPENISA_RV32M1_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_E_BASE_ADDRESS,
	.port_base = PORTE,
#ifdef DT_ALIAS_GPIO_E_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
	.irq_config_func = gpio_rv32m1_porte_init,
#ifdef DT_ALIAS_GPIO_E_CLOCK_CONTROLLER
	.clock_controller = DT_ALIAS_GPIO_E_CLOCK_CONTROLLER,
	.clock_subsys = (clock_control_subsys_t)
			DT_ALIAS_GPIO_E_CLOCK_NAME,
#else
	.clock_controller = NULL,
#endif
};

static struct gpio_rv32m1_data gpio_rv32m1_porte_data;

DEVICE_AND_API_INIT(gpio_rv32m1_porte, DT_ALIAS_GPIO_E_LABEL,
		    gpio_rv32m1_init,
		    &gpio_rv32m1_porte_data, &gpio_rv32m1_porte_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_rv32m1_driver_api);

static int gpio_rv32m1_porte_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_E_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_E_IRQ_0,
		    0,
		    gpio_rv32m1_port_isr, DEVICE_GET(gpio_rv32m1_porte), 0);

	irq_enable(DT_ALIAS_GPIO_E_IRQ_0);

	return 0;
#else
	return -1;
#endif
}
#endif /* CONFIG_GPIO_RV32M1_PORTE */
