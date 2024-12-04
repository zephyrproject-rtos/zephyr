/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2017, 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/gpio/nxp-kinetis-gpio.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <fsl_common.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

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
};

static int gpio_mcux_configure(const struct device *dev,
			       gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	PORT_Type *port_base = config->port_base;
	uint32_t mask = 0U;
	uint32_t pcr = 0U;

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

	/* Set PCR mux to GPIO for the pin we are configuring */
	mask |= PORT_PCR_MUX_MASK;
	pcr |= PORT_PCR_MUX(PORT_MUX_GPIO);

#if defined(FSL_FEATURE_PORT_HAS_INPUT_BUFFER) && FSL_FEATURE_PORT_HAS_INPUT_BUFFER
	/* Enable digital input buffer */
	pcr |= PORT_PCR_IBE_MASK;
#endif

	/* Now do the PORT module. Figure out the pullup/pulldown
	 * configuration, but don't write it to the PCR register yet.
	 */
	mask |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	if ((flags & GPIO_PULL_UP) != 0) {
		/* Enable the pull and select the pullup resistor. */
		pcr |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		/* Enable the pull and select the pulldown resistor, deselect
		 * the pullup resistor.
		 */
		pcr |= PORT_PCR_PE_MASK;
	}

#if defined(FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH) && FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH
	/* Determine the drive strength */
	switch (flags & KINETIS_GPIO_DS_MASK) {
	case KINETIS_GPIO_DS_DFLT:
		/* Default is low drive strength */
		mask |= PORT_PCR_DSE_MASK;
		break;
	case KINETIS_GPIO_DS_ALT:
		/* Alternate is high drive strength */
		pcr |= PORT_PCR_DSE_MASK;
		break;
	default:
		return -ENOTSUP;
	}
#endif /* defined(FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH) && FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH */

	/* Accessing by pin, we only need to write one PCR register. */
	port_base->PCR[pin] = (port_base->PCR[pin] & ~mask) | pcr;

	return 0;
}

static int gpio_mcux_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PDIR;

	return 0;
}

static int gpio_mcux_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PDOR = (gpio_base->PDOR & ~mask) | (mask & value);

	return 0;
}

static int gpio_mcux_port_set_bits_raw(const struct device *dev,
				       uint32_t mask)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PSOR = mask;

	return 0;
}

static int gpio_mcux_port_clear_bits_raw(const struct device *dev,
					 uint32_t mask)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PCOR = mask;

	return 0;
}

static int gpio_mcux_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->PTOR = mask;

	return 0;
}

#if !(defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
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
			default:
				return -EINVAL;
			}
		}
	}

	return PORT_PCR_IRQC(port_interrupt);
}
#endif  /* !defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT */

#if (defined(FSL_FEATURE_GPIO_HAS_INTERRUPT_CHANNEL_SELECT) && \
		FSL_FEATURE_GPIO_HAS_INTERRUPT_CHANNEL_SELECT)

#define GPIO_MCUX_INTERRUPT_DISABLED		0
#define GPIO_MCUX_INTERRUPT_LOGIC_0		0x8
#define GPIO_MCUX_INTERRUPT_RISING_EDGE		0x9
#define GPIO_MCUX_INTERRUPT_FALLING_EDGE	0xA
#define GPIO_MCUX_INTERRUPT_BOTH_EDGE		0xB
#define GPIO_MCUX_INTERRUPT_LOGIC_1		0xC

static uint32_t get_gpio_icr_irqc_value_from_flags(const struct device *dev,
						   uint32_t pin,
						   enum gpio_int_mode mode,
						   enum gpio_int_trig trig)
{
	uint8_t gpio_interrupt = 0;

	if (mode == GPIO_INT_MODE_DISABLED) {
		gpio_interrupt = GPIO_MCUX_INTERRUPT_DISABLED;
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig == GPIO_INT_TRIG_LOW) {
				gpio_interrupt = GPIO_MCUX_INTERRUPT_LOGIC_0;
			} else {
				gpio_interrupt = GPIO_MCUX_INTERRUPT_LOGIC_1;
			}
		} else {
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				gpio_interrupt = GPIO_MCUX_INTERRUPT_FALLING_EDGE;
				break;
			case GPIO_INT_TRIG_HIGH:
				gpio_interrupt = GPIO_MCUX_INTERRUPT_RISING_EDGE;
				break;
			case GPIO_INT_TRIG_BOTH:
				gpio_interrupt = GPIO_MCUX_INTERRUPT_BOTH_EDGE;
				break;
			default:
				return -EINVAL;
			}
		}
	}

	return GPIO_ICR_IRQC(gpio_interrupt);
}
#endif  /* (defined(FSL_FEATURE_GPIO_HAS_INTERRUPT_CHANNEL_SELECT) */

static int gpio_mcux_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin, enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	PORT_Type *port_base = config->port_base;

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

#if !(defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
	uint32_t pcr = get_port_pcr_irqc_value_from_flags(dev, pin, mode, trig);

	port_base->PCR[pin] = (port_base->PCR[pin] & ~PORT_PCR_IRQC_MASK) | pcr;
#elif (defined(FSL_FEATURE_GPIO_HAS_INTERRUPT_CHANNEL_SELECT) && \
		FSL_FEATURE_GPIO_HAS_INTERRUPT_CHANNEL_SELECT)
	uint32_t icr = get_gpio_icr_irqc_value_from_flags(dev, pin, mode, trig);

	gpio_base->ICR[pin] = (gpio_base->ICR[pin] & ~GPIO_ICR_IRQC_MASK) | icr;
#endif  /* !(defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) */

	return 0;
}

static int gpio_mcux_manage_callback(const struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct gpio_mcux_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_mcux_port_isr(const struct device *dev)
{
	const struct gpio_mcux_config *config = dev->config;
	struct gpio_mcux_data *data = dev->data;
	uint32_t int_status;

#if !(defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) && FSL_FEATURE_PORT_HAS_NO_INTERRUPT)
	int_status = config->port_base->ISFR;

	/* Clear the port interrupts */
	config->port_base->ISFR = int_status;
#elif (defined(FSL_FEATURE_GPIO_HAS_INTERRUPT_CHANNEL_SELECT) && \
		FSL_FEATURE_GPIO_HAS_INTERRUPT_CHANNEL_SELECT)
	int_status = config->gpio_base->ISFR[0];

	/* Clear the gpio interrupts */
	config->gpio_base->ISFR[0] = int_status;
#else
	int_status = 0U;
	ARG_UNUSED(config);
#endif  /* !(defined(FSL_FEATURE_PORT_HAS_NO_INTERRUPT) */

	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

#define GPIO_HAS_SHARED_IRQ DT_HAS_COMPAT_STATUS_OKAY(nxp_gpio_cluster)

#if GPIO_HAS_SHARED_IRQ
static void gpio_mcux_shared_cluster_isr(const struct device *ports[])
{
	const struct device **current_port = &ports[0];

	while (*current_port != NULL) {
		gpio_mcux_port_isr(*current_port);
		current_port++;
	}
}

#define CLUSTER_ARRAY_ELEMENT(node_id) DEVICE_DT_GET(node_id),

#define GPIO_MCUX_CLUSTER_INIT(node_id)								\
	const struct device *shared_array##node_id[DT_CHILD_NUM_STATUS_OKAY(node_id) + 1] =	\
		{DT_FOREACH_CHILD_STATUS_OKAY(node_id, CLUSTER_ARRAY_ELEMENT) NULL};	\
												\
	static int gpio_mcux_shared_interrupt_init##node_id(void)	\
	{								\
		IRQ_CONNECT(DT_IRQN(node_id),				\
			    DT_IRQ(node_id, priority),			\
			    gpio_mcux_shared_cluster_isr,		\
			    shared_array##node_id, 0);			\
		irq_enable(DT_IRQN(node_id));				\
									\
		return 0;						\
	}								\
	SYS_INIT(gpio_mcux_shared_interrupt_init##node_id, POST_KERNEL, 0);

DT_FOREACH_STATUS_OKAY(nxp_gpio_cluster, GPIO_MCUX_CLUSTER_INIT)
#endif

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_mcux_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_mcux_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		*inputs = map & (~gpio_base->PDDR);
	}

	if (outputs != NULL) {
		*outputs = map & gpio_base->PDDR;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static DEVICE_API(gpio, gpio_mcux_driver_api) = {
	.pin_configure = gpio_mcux_configure,
	.port_get_raw = gpio_mcux_port_get_raw,
	.port_set_masked_raw = gpio_mcux_port_set_masked_raw,
	.port_set_bits_raw = gpio_mcux_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mcux_port_clear_bits_raw,
	.port_toggle_bits = gpio_mcux_port_toggle_bits,
	.pin_interrupt_configure = gpio_mcux_pin_interrupt_configure,
	.manage_callback = gpio_mcux_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_mcux_port_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

#define GPIO_MCUX_IRQ_INIT(n)						\
	do {								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    gpio_mcux_port_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
									\
		irq_enable(DT_INST_IRQN(n));				\
	} while (false)

#define GPIO_PORT_BASE_ADDR(n) DT_REG_ADDR(DT_INST_PHANDLE(n, nxp_kinetis_port))

#define GPIO_DEVICE_INIT_MCUX(n)					\
	static int gpio_mcux_port## n ## _init(const struct device *dev); \
									\
	static const struct gpio_mcux_config gpio_mcux_port## n ## _config = {\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.gpio_base = (GPIO_Type *) DT_INST_REG_ADDR(n),		\
		.port_base = (PORT_Type *) GPIO_PORT_BASE_ADDR(n),	\
		.flags = UTIL_AND(UTIL_OR(DT_INST_IRQ_HAS_IDX(n, 0),	\
				GPIO_HAS_SHARED_IRQ), GPIO_INT_ENABLE), \
	};								\
									\
	static struct gpio_mcux_data gpio_mcux_port## n ##_data;	\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			    gpio_mcux_port## n ##_init,			\
			    NULL,					\
			    &gpio_mcux_port## n ##_data,		\
			    &gpio_mcux_port## n##_config,		\
			    POST_KERNEL,				\
			    CONFIG_GPIO_INIT_PRIORITY,			\
			    &gpio_mcux_driver_api);			\
									\
	static int gpio_mcux_port## n ##_init(const struct device *dev)	\
	{								\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0),			\
			(GPIO_MCUX_IRQ_INIT(n);))			\
		return 0;						\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_DEVICE_INIT_MCUX)
