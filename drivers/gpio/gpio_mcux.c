/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_port.h>

#include "gpio_utils.h"

struct gpio_mcux_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *gpio_base;
	PORT_Type *port_base;
	unsigned int flags;
};

struct gpio_mcux_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

static int gpio_mcux_configure(struct device *dev,
			       gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;
	PORT_Type *port_base = config->port_base;
	u32_t mask = 0U;
	u32_t pcr = 0U;

	/* Check for an invalid pin number */
	if (pin >= ARRAY_SIZE(port_base->PCR)) {
		return -EINVAL;
	}

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
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

	/* Accessing by pin, we only need to write one PCR register. */
	port_base->PCR[pin] = (port_base->PCR[pin] & ~mask) | pcr;

	return 0;
}

static int gpio_mcux_port_get_raw(struct device *dev, u32_t *value)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PDIR;

	return 0;
}

static int gpio_mcux_port_set_masked_raw(struct device *dev, u32_t mask,
					 u32_t value)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PDOR = (gpio_base->PDOR & ~mask) | (mask & value);

	return 0;
}

static int gpio_mcux_port_set_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PSOR = mask;

	return 0;
}

static int gpio_mcux_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PCOR = mask;

	return 0;
}

static int gpio_mcux_port_toggle_bits(struct device *dev, u32_t mask)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PTOR = mask;

	return 0;
}

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

static int gpio_mcux_pin_interrupt_configure(struct device *dev,
					     gpio_pin_t pin, enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_mcux_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;
	PORT_Type *port_base = config->port_base;
	struct gpio_mcux_data *data = dev->driver_data;

	/* Check for an invalid pin number */
	if (pin >= ARRAY_SIZE(port_base->PCR)) {
		return -EINVAL;
	}

	/* Check for an invalid pin configuration */
	if ((mode != GPIO_INT_MODE_DISABLED) &&
	    ((gpio_base->PDDR & BIT(pin)) != 0)) {
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

static int gpio_mcux_manage_callback(struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct gpio_mcux_data *data = dev->driver_data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_mcux_enable_callback(struct device *dev,
				     gpio_pin_t pin)
{
	struct gpio_mcux_data *data = dev->driver_data;

	data->pin_callback_enables |= BIT(pin);

	return 0;
}

static int gpio_mcux_disable_callback(struct device *dev,
				      gpio_pin_t pin)
{
	struct gpio_mcux_data *data = dev->driver_data;

	data->pin_callback_enables &= ~BIT(pin);

	return 0;
}

static void gpio_mcux_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_mcux_config *config = dev->config->config_info;
	struct gpio_mcux_data *data = dev->driver_data;
	u32_t enabled_int, int_status;

	int_status = config->port_base->ISFR;
	enabled_int = int_status & data->pin_callback_enables;

	/* Clear the port interrupts */
	config->port_base->ISFR = enabled_int;

	gpio_fire_callbacks(&data->callbacks, dev, enabled_int);
}


static const struct gpio_driver_api gpio_mcux_driver_api = {
	.pin_configure = gpio_mcux_configure,
	.port_get_raw = gpio_mcux_port_get_raw,
	.port_set_masked_raw = gpio_mcux_port_set_masked_raw,
	.port_set_bits_raw = gpio_mcux_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mcux_port_clear_bits_raw,
	.port_toggle_bits = gpio_mcux_port_toggle_bits,
	.pin_interrupt_configure = gpio_mcux_pin_interrupt_configure,
	.manage_callback = gpio_mcux_manage_callback,
	.enable_callback = gpio_mcux_enable_callback,
	.disable_callback = gpio_mcux_disable_callback,
};

#ifdef CONFIG_GPIO_MCUX_PORTA
static int gpio_mcux_porta_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_porta_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_0_NXP_KINETIS_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_A_BASE_ADDRESS,
	.port_base = PORTA,
#ifdef DT_ALIAS_GPIO_A_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_mcux_data gpio_mcux_porta_data;

DEVICE_AND_API_INIT(gpio_mcux_porta, DT_ALIAS_GPIO_A_LABEL,
		    gpio_mcux_porta_init,
		    &gpio_mcux_porta_data, &gpio_mcux_porta_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_porta_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_A_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_A_IRQ_0, DT_ALIAS_GPIO_A_IRQ_0_PRIORITY,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_porta), 0);

	irq_enable(DT_ALIAS_GPIO_A_IRQ_0);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTA */

#ifdef CONFIG_GPIO_MCUX_PORTB
static int gpio_mcux_portb_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_portb_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_1_NXP_KINETIS_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_B_BASE_ADDRESS,
	.port_base = PORTB,
#ifdef DT_ALIAS_GPIO_B_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_mcux_data gpio_mcux_portb_data;

DEVICE_AND_API_INIT(gpio_mcux_portb, DT_ALIAS_GPIO_B_LABEL,
		    gpio_mcux_portb_init,
		    &gpio_mcux_portb_data, &gpio_mcux_portb_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_portb_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_B_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_B_IRQ_0, DT_ALIAS_GPIO_B_IRQ_0_PRIORITY,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_portb), 0);

	irq_enable(DT_ALIAS_GPIO_B_IRQ_0);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTB */

#ifdef CONFIG_GPIO_MCUX_PORTC
static int gpio_mcux_portc_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_portc_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_2_NXP_KINETIS_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_C_BASE_ADDRESS,
	.port_base = PORTC,
#ifdef DT_ALIAS_GPIO_C_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_mcux_data gpio_mcux_portc_data;

DEVICE_AND_API_INIT(gpio_mcux_portc, DT_ALIAS_GPIO_C_LABEL,
		    gpio_mcux_portc_init,
		    &gpio_mcux_portc_data, &gpio_mcux_portc_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_portc_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_C_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_C_IRQ_0, DT_ALIAS_GPIO_C_IRQ_0_PRIORITY,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_portc), 0);

	irq_enable(DT_ALIAS_GPIO_C_IRQ_0);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTC */

#ifdef CONFIG_GPIO_MCUX_PORTD
static int gpio_mcux_portd_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_portd_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_3_NXP_KINETIS_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_D_BASE_ADDRESS,
	.port_base = PORTD,
#ifdef DT_ALIAS_GPIO_D_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_mcux_data gpio_mcux_portd_data;

DEVICE_AND_API_INIT(gpio_mcux_portd, DT_ALIAS_GPIO_D_LABEL,
		    gpio_mcux_portd_init,
		    &gpio_mcux_portd_data, &gpio_mcux_portd_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_portd_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_D_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_D_IRQ_0, DT_ALIAS_GPIO_D_IRQ_0_PRIORITY,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_portd), 0);

	irq_enable(DT_ALIAS_GPIO_D_IRQ_0);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTD */

#ifdef CONFIG_GPIO_MCUX_PORTE
static int gpio_mcux_porte_init(struct device *dev);

static const struct gpio_mcux_config gpio_mcux_porte_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_4_NXP_KINETIS_GPIO_NGPIOS),
	},
	.gpio_base = (GPIO_Type *) DT_ALIAS_GPIO_E_BASE_ADDRESS,
	.port_base = PORTE,
#ifdef DT_ALIAS_GPIO_E_IRQ_0
	.flags = GPIO_INT_ENABLE,
#else
	.flags = 0,
#endif
};

static struct gpio_mcux_data gpio_mcux_porte_data;

DEVICE_AND_API_INIT(gpio_mcux_porte, DT_ALIAS_GPIO_E_LABEL,
		    gpio_mcux_porte_init,
		    &gpio_mcux_porte_data, &gpio_mcux_porte_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_driver_api);

static int gpio_mcux_porte_init(struct device *dev)
{
#ifdef DT_ALIAS_GPIO_E_IRQ_0
	IRQ_CONNECT(DT_ALIAS_GPIO_E_IRQ_0, DT_ALIAS_GPIO_E_IRQ_0_PRIORITY,
		    gpio_mcux_port_isr, DEVICE_GET(gpio_mcux_porte), 0);

	irq_enable(DT_ALIAS_GPIO_E_IRQ_0);
#endif
	return 0;
}
#endif /* CONFIG_GPIO_MCUX_PORTE */
