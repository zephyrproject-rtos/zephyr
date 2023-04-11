/*
 * Copyright 2017-2020,2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_gpio

/** @file
 * @brief GPIO driver for LPC54XXX family
 *
 * Note:
 * - fsl_pint internally tries to manage interrupts, but this is not used (e.g.
 *   s_pintCallback), Zephyr's interrupt management system is used in place.
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <fsl_common.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/nxp_pint.h>
#include <fsl_gpio.h>
#include <fsl_device_registers.h>

/* Interrupt sources, matching int-source enum in DTS binding definition */
#define INT_SOURCE_PINT 0
#define INT_SOURCE_INTA 1
#define INT_SOURCE_INTB 2
#define INT_SOURCE_NONE 3

struct gpio_mcux_lpc_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *gpio_base;
	uint8_t int_source;
#ifdef IOPCTL
	IOPCTL_Type *pinmux_base;
#else
	IOCON_Type *pinmux_base;
#endif
	uint32_t port_no;
	clock_ip_name_t clock_ip_name;
};

struct gpio_mcux_lpc_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
};

static int gpio_mcux_lpc_configure(const struct device *dev, gpio_pin_t pin,
				   gpio_flags_t flags)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port = config->port_no;

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

#ifdef IOPCTL /* RT600 and RT500 series */
	IOPCTL_Type *pinmux_base = config->pinmux_base;
	volatile uint32_t *pinconfig = (volatile uint32_t *)&(pinmux_base->PIO[port][pin]);

	/*
	 * Enable input buffer for both input and output pins, it costs
	 * nothing and allows values to be read back.
	 */
	*pinconfig |= IOPCTL_PIO_INBUF_EN;

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		*pinconfig |= IOPCTL_PIO_PSEDRAIN_EN;
	} else {
		*pinconfig &= ~IOPCTL_PIO_PSEDRAIN_EN;
	}
	/* Select GPIO mux for this pin (func 0 is always GPIO) */
	*pinconfig &= ~(IOPCTL_PIO_FSEL_MASK);

#else /* LPC SOCs */
	volatile uint32_t *pinconfig;
	IOCON_Type *pinmux_base;

	pinmux_base = config->pinmux_base;
	pinconfig = (volatile uint32_t *)&(pinmux_base->PIO[port][pin]);

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		/* Set ODE bit. */
		*pinconfig |= IOCON_PIO_OD_MASK;
	}

	if ((flags & GPIO_INPUT) != 0) {
		/* Set DIGIMODE bit */
		*pinconfig |= IOCON_PIO_DIGIMODE_MASK;
	}
	/* Select GPIO mux for this pin (func 0 is always GPIO) */
	*pinconfig &= ~(IOCON_PIO_FUNC_MASK);
#endif

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
#ifdef IOPCTL /* RT600 and RT500 series */
		*pinconfig |= IOPCTL_PIO_PUPD_EN;
		if ((flags & GPIO_PULL_UP) != 0) {
			*pinconfig |= IOPCTL_PIO_PULLUP_EN;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pinconfig &= ~(IOPCTL_PIO_PULLUP_EN);
		}
#else /* LPC SOCs */

		*pinconfig &= ~(IOCON_PIO_MODE_PULLUP|IOCON_PIO_MODE_PULLDOWN);
		if ((flags & GPIO_PULL_UP) != 0) {
			*pinconfig |= IOCON_PIO_MODE_PULLUP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pinconfig |= IOCON_PIO_MODE_PULLDOWN;
		}
#endif
	}

	/* supports access by pin now,you can add access by port when needed */
	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		gpio_base->SET[port] = BIT(pin);
	}

	if (flags & GPIO_OUTPUT_INIT_LOW) {
		gpio_base->CLR[port] = BIT(pin);
	}

	/* input-0,output-1 */
	WRITE_BIT(gpio_base->DIR[port], pin, flags & GPIO_OUTPUT);

	return 0;
}

static int gpio_mcux_lpc_port_get_raw(const struct device *dev,
				      uint32_t *value)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PIN[config->port_no];

	return 0;
}

static int gpio_mcux_lpc_port_set_masked_raw(const struct device *dev,
					     uint32_t mask,
					     uint32_t value)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port = config->port_no;

	/* Writing 0 allows R+W, 1 disables the pin */
	gpio_base->MASK[port] = ~mask;
	gpio_base->MPIN[port] = value;
	/* Enable back the pins, user won't assume pins remain masked*/
	gpio_base->MASK[port] = 0U;

	return 0;
}

static int gpio_mcux_lpc_port_set_bits_raw(const struct device *dev,
					   uint32_t mask)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->SET[config->port_no] = mask;

	return 0;
}

static int gpio_mcux_lpc_port_clear_bits_raw(const struct device *dev,
					     uint32_t mask)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->CLR[config->port_no] = mask;

	return 0;
}

static int gpio_mcux_lpc_port_toggle_bits(const struct device *dev,
					  uint32_t mask)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->NOT[config->port_no] = mask;

	return 0;
}


/* Called by PINT when pin interrupt fires */
static void gpio_mcux_lpc_pint_cb(uint8_t pin, void *user)
{
	const struct device *dev = user;
	const struct gpio_mcux_lpc_config *config = dev->config;
	struct gpio_mcux_lpc_data *data = dev->data;
	uint32_t gpio_pin;

	/* Subtract port number times 32 from pin number to get GPIO API
	 * pin number.
	 */
	gpio_pin = pin - (config->port_no * 32);

	gpio_fire_callbacks(&data->callbacks, dev, BIT(gpio_pin));
}


/* Installs interrupt handler using PINT interrupt controller. */
static int gpio_mcux_lpc_pint_interrupt_cfg(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	enum nxp_pint_trigger interrupt_mode = NXP_PINT_NONE;
	uint32_t port = config->port_no;
	int ret;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		nxp_pint_pin_disable((port * 32) + pin);
		return 0;
	case GPIO_INT_MODE_LEVEL:
		if (trig == GPIO_INT_TRIG_HIGH) {
			interrupt_mode = NXP_PINT_HIGH;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			interrupt_mode = NXP_PINT_LOW;
		} else {
			return -ENOTSUP;
		}
		break;
	case GPIO_INT_MODE_EDGE:
		if (trig == GPIO_INT_TRIG_HIGH) {
			interrupt_mode = NXP_PINT_RISING;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			interrupt_mode = NXP_PINT_FALLING;
		} else {
			interrupt_mode = NXP_PINT_BOTH;
		}
		break;
	default:
		return -ENOTSUP;
	}

	/* PINT treats GPIO pins as continuous. Each port has 32 pins */
	ret = nxp_pint_pin_enable((port * 32) + pin, interrupt_mode);
	if (ret < 0) {
		return ret;
	}
	/* Install callback */
	return nxp_pint_pin_set_callback((port * 32) + pin,
					  gpio_mcux_lpc_pint_cb,
					  (struct device *)dev);
}


#if (defined(FSL_FEATURE_GPIO_HAS_INTERRUPT) && FSL_FEATURE_GPIO_HAS_INTERRUPT)

static int gpio_mcux_lpc_module_interrupt_cfg(const struct device *dev,
					      gpio_pin_t pin,
					      enum gpio_int_mode mode,
					      enum gpio_int_trig trig)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	gpio_interrupt_index_t int_idx;
	gpio_interrupt_config_t pin_config;

	if (config->int_source == INT_SOURCE_NONE) {
		return -ENOTSUP;
	}

	/* Route interrupt to source A or B based on interrupt source */
	int_idx = (config->int_source == INT_SOURCE_INTA) ?
			kGPIO_InterruptA : kGPIO_InterruptB;

	/* Disable interrupt if requested */
	if (mode ==  GPIO_INT_MODE_DISABLED) {
		GPIO_PinDisableInterrupt(config->gpio_base, config->port_no, pin, int_idx);
		return 0;
	}

	/* Set pin interrupt level */
	if (mode == GPIO_INT_MODE_LEVEL) {
		pin_config.mode = kGPIO_PinIntEnableLevel;
	} else if (mode == GPIO_INT_MODE_EDGE) {
		pin_config.mode = kGPIO_PinIntEnableEdge;
	} else {
		return -ENOTSUP;
	}

	/* Set pin interrupt trigger */
	if (trig == GPIO_INT_TRIG_HIGH) {
		pin_config.polarity = kGPIO_PinIntEnableHighOrRise;
	} else if (trig == GPIO_INT_TRIG_LOW) {
		pin_config.polarity = kGPIO_PinIntEnableLowOrFall;
	} else {
		return -ENOTSUP;
	}

	/* Enable interrupt with new configuration */
	GPIO_SetPinInterruptConfig(config->gpio_base, config->port_no, pin, &pin_config);
	GPIO_PinEnableInterrupt(config->gpio_base, config->port_no, pin, int_idx);
	return 0;
}

/* GPIO module interrupt handler */
void gpio_mcux_lpc_module_isr(const struct device *dev)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	struct gpio_mcux_lpc_data *data = dev->data;
	uint32_t status;

	status = GPIO_PortGetInterruptStatus(config->gpio_base,
					config->port_no,
					config->int_source == INT_SOURCE_INTA ?
					kGPIO_InterruptA : kGPIO_InterruptB);
	GPIO_PortClearInterruptFlags(config->gpio_base,
					config->port_no,
					config->int_source == INT_SOURCE_INTA ?
					kGPIO_InterruptA : kGPIO_InterruptB,
					status);
	gpio_fire_callbacks(&data->callbacks, dev, status);
}

#endif /* FSL_FEATURE_GPIO_HAS_INTERRUPT */


static int gpio_mcux_lpc_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port = config->port_no;

	/* Ensure pin used as interrupt is set as input*/
	if ((mode & GPIO_INT_ENABLE) &&
		((gpio_base->DIR[port] & BIT(pin)) != 0)) {
		return -ENOTSUP;
	}
	if (config->int_source == INT_SOURCE_PINT) {
		return gpio_mcux_lpc_pint_interrupt_cfg(dev, pin, mode, trig);
	}
#if (defined(FSL_FEATURE_GPIO_HAS_INTERRUPT) && FSL_FEATURE_GPIO_HAS_INTERRUPT)
	return gpio_mcux_lpc_module_interrupt_cfg(dev, pin, mode, trig);
#else
	return -ENOTSUP;
#endif
}

static int gpio_mcux_lpc_manage_cb(const struct device *port,
				   struct gpio_callback *callback, bool set)
{
	struct gpio_mcux_lpc_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_mcux_lpc_init(const struct device *dev)
{
	const struct gpio_mcux_lpc_config *config = dev->config;

	GPIO_PortInit(config->gpio_base, config->port_no);

	return 0;
}

static const struct gpio_driver_api gpio_mcux_lpc_driver_api = {
	.pin_configure = gpio_mcux_lpc_configure,
	.port_get_raw = gpio_mcux_lpc_port_get_raw,
	.port_set_masked_raw = gpio_mcux_lpc_port_set_masked_raw,
	.port_set_bits_raw = gpio_mcux_lpc_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mcux_lpc_port_clear_bits_raw,
	.port_toggle_bits = gpio_mcux_lpc_port_toggle_bits,
	.pin_interrupt_configure = gpio_mcux_lpc_pin_interrupt_configure,
	.manage_callback = gpio_mcux_lpc_manage_cb,
};

static const clock_ip_name_t gpio_clock_names[] = GPIO_CLOCKS;

#ifdef IOPCTL
#define PINMUX_BASE	IOPCTL
#else
#define PINMUX_BASE	IOCON
#endif

#define GPIO_MCUX_LPC_MODULE_IRQ_CONNECT(inst)						\
	do {										\
		IRQ_CONNECT(DT_INST_IRQ(inst, irq),					\
			DT_INST_IRQ(inst, priority),					\
			gpio_mcux_lpc_module_isr, DEVICE_DT_INST_GET(inst), 0);		\
		irq_enable(DT_INST_IRQ(inst, irq));					\
	} while (false)

#define GPIO_MCUX_LPC_MODULE_IRQ(inst)							\
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(inst, 0),					\
		(GPIO_MCUX_LPC_MODULE_IRQ_CONNECT(inst)))


#define GPIO_MCUX_LPC(n)								\
	static int lpc_gpio_init_##n(const struct device *dev);				\
											\
	static const struct gpio_mcux_lpc_config gpio_mcux_lpc_config_##n = {		\
		.common = {								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),		\
		},									\
		.gpio_base = GPIO,							\
		.pinmux_base = PINMUX_BASE,						\
		.int_source = DT_INST_ENUM_IDX(n, int_source),				\
		.port_no = DT_INST_PROP(n, port),					\
		.clock_ip_name = gpio_clock_names[DT_INST_PROP(n, port)],		\
	};										\
											\
	static struct gpio_mcux_lpc_data gpio_mcux_lpc_data_##n;			\
											\
	DEVICE_DT_INST_DEFINE(n, lpc_gpio_init_##n, NULL,				\
		    &gpio_mcux_lpc_data_##n,						\
		    &gpio_mcux_lpc_config_##n, PRE_KERNEL_1,				\
		    CONFIG_GPIO_INIT_PRIORITY,						\
		    &gpio_mcux_lpc_driver_api);						\
											\
	static int lpc_gpio_init_##n(const struct device *dev)				\
	{										\
		gpio_mcux_lpc_init(dev);						\
		GPIO_MCUX_LPC_MODULE_IRQ(n);						\
											\
		return 0;								\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_MCUX_LPC)
