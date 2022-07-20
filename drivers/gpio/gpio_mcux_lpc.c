/*
 * Copyright (c) 2017-2020, NXP
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
#include <soc.h>
#include <fsl_common.h>
#include "gpio_utils.h"
#include <fsl_gpio.h>
#include <fsl_pint.h>
#include <fsl_inputmux.h>
#include <fsl_device_registers.h>

#define PIN_TO_INPUT_MUX_CONNECTION(port, pin) \
	((PINTSEL_PMUX_ID << PMUX_SHIFT) + (32 * port) + (pin))

#ifndef FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS
#define FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS 0
#endif
#ifndef FSL_FEATURE_SECPINT_NUMBER_OF_CONNECTED_OUTPUTS
#define FSL_FEATURE_SECPINT_NUMBER_OF_CONNECTED_OUTPUTS 0
#endif

#define NO_PINT_INT                                                                                \
	MAX(FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS,                                          \
	    FSL_FEATURE_SECPINT_NUMBER_OF_CONNECTED_OUTPUTS)

struct gpio_mcux_lpc_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *gpio_base;
	PINT_Type *pint_base;
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
	/* pin association with PINT id */
	pint_pin_int_t pint_id[32];
	/* ISR allocated in device tree to this port */
	uint32_t isr_list[8];
	/* index to to table above */
	uint32_t isr_list_idx;
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

static void gpio_mcux_lpc_port_isr(const struct device *dev)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	struct gpio_mcux_lpc_data *data = dev->data;
	uint32_t enabled_int;
	uint32_t int_flags;
	uint32_t pin;

	for (pin = 0; pin < 32; pin++) {
		if (data->pint_id[pin] != NO_PINT_INT) {
			int_flags = PINT_PinInterruptGetStatus(
				config->pint_base, data->pint_id[pin]);
			enabled_int = int_flags << pin;

			if (int_flags) {
				PINT_PinInterruptClrStatus(config->pint_base,
							   data->pint_id[pin]);
			}

			gpio_fire_callbacks(&data->callbacks, dev, enabled_int);
		}
	}
}

static uint32_t get_free_isr(struct gpio_mcux_lpc_data *data)
{
	uint32_t i;
	uint32_t isr;

	for (i = 0; i < data->isr_list_idx; i++) {
		if (data->isr_list[i] != -1) {
			isr = data->isr_list[i];
			data->isr_list[i] = -1;
			return isr;
		}
	}

	return -EINVAL;
}

/* Function configures INPUTMUX device to route pin interrupts to a certain
 * PINT. PINT no. is unknown, rather it's determined from ISR no.
 */
static uint32_t attach_pin_to_isr(uint32_t port, uint32_t pin, uint32_t isr_no)
{
	uint32_t pint_idx;
	/* Connect trigger sources to PINT */
	INPUTMUX_Init(INPUTMUX);

	/* Code asumes PIN_INT values are grouped [0..3] and [4..7].
	 * This scenario is true in LPC54xxx/LPC55xxx.
	 */
#if (FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS > 8)
	#error having more than 8 PINT IRQs not supported in driver
#elif (FSL_FEATURE_PINT_NUMBER_OF_CONNECTED_OUTPUTS > 4)
	if (isr_no < PIN_INT4_IRQn) {
		pint_idx = isr_no - PIN_INT0_IRQn;
	} else {
		pint_idx = isr_no - PIN_INT4_IRQn + 4;
	}
#else
	pint_idx = isr_no - PIN_INT0_IRQn;
#endif

	INPUTMUX_AttachSignal(INPUTMUX, pint_idx,
			      PIN_TO_INPUT_MUX_CONNECTION(port, pin));

	/* Turnoff clock to inputmux to save power. Clock is only needed to make
	 * changes. Can be turned off after.
	 */
	INPUTMUX_Deinit(INPUTMUX);

	return pint_idx;
}

static void gpio_mcux_lpc_port_isr(const struct device *dev);


static int gpio_mcux_lpc_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	const struct gpio_mcux_lpc_config *config = dev->config;
	struct gpio_mcux_lpc_data *data = dev->data;
	pint_pin_enable_t interruptMode = kPINT_PinIntEnableNone;
	GPIO_Type *gpio_base = config->gpio_base;
	uint32_t port = config->port_no;
	uint32_t isr;
	uint32_t pint_idx;
	static bool pint_inited;

	/* Ensure pin used as interrupt is set as input*/
	if ((mode & GPIO_INT_ENABLE) &&
		((gpio_base->DIR[port] & BIT(pin)) != 0)) {
		return -ENOTSUP;
	}

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		interruptMode = kPINT_PinIntEnableNone;
		break;
	case GPIO_INT_MODE_LEVEL:
		if (trig == GPIO_INT_TRIG_HIGH) {
			interruptMode = kPINT_PinIntEnableHighLevel;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			interruptMode = kPINT_PinIntEnableLowLevel;
		} else {
			return -ENOTSUP;
		}
		break;
	case GPIO_INT_MODE_EDGE:
		if (trig == GPIO_INT_TRIG_HIGH) {
			interruptMode = kPINT_PinIntEnableRiseEdge;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			interruptMode = kPINT_PinIntEnableFallEdge;
		} else {
			interruptMode = kPINT_PinIntEnableBothEdges;
		}
		break;
	default:
		return -ENOTSUP;
	}

	/* First time calling this function routes PIN->PINT->INPUTMUX->NVIC */
	if (data->pint_id[pin] == NO_PINT_INT) {
		isr = get_free_isr(data);
		if (isr == -EINVAL) {
			/* Didn't find any free interrupt in this port */
			return -EBUSY;
		}
		pint_idx = attach_pin_to_isr(port, pin, isr);
		data->pint_id[pin] = pint_idx;
	}

	if (!pint_inited) {
		PINT_Init(config->pint_base);
		pint_inited = true;
	}
	PINT_PinInterruptConfig(config->pint_base, data->pint_id[pin],
		interruptMode,
		(pint_cb_t)gpio_mcux_lpc_port_isr);

	return 0;
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
	struct gpio_mcux_lpc_data *data = dev->data;
	int i;

	GPIO_PortInit(config->gpio_base, config->port_no);

	for (i = 0; i < 32; i++) {
		data->pint_id[i] = NO_PINT_INT;
	}

	data->isr_list_idx = 0;

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

#define GPIO_MCUX_LPC_IRQ_CONNECT(n, m)							\
	do {										\
		struct gpio_mcux_lpc_data *data = dev->data;				\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq),				\
			    DT_INST_IRQ_BY_IDX(n, m, priority),				\
			    gpio_mcux_lpc_port_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));				\
		data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(n, m, irq);	\
	} while (false)

#define GPIO_MCUX_LPC_IRQ(n, m)								\
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, m), (GPIO_MCUX_LPC_IRQ_CONNECT(n, m)), ())


#define GPIO_MCUX_LPC(n)								\
	static int lpc_gpio_init_##n(const struct device *dev);				\
											\
	static const struct gpio_mcux_lpc_config gpio_mcux_lpc_config_##n = {		\
		.common = {								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),		\
		},									\
		.gpio_base = GPIO,							\
		.pint_base = PINT, /* TODO: SECPINT issue #16330 */			\
		.pinmux_base = PINMUX_BASE,						\
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
											\
		GPIO_MCUX_LPC_IRQ(n, 0);						\
		GPIO_MCUX_LPC_IRQ(n, 1);						\
		GPIO_MCUX_LPC_IRQ(n, 2);						\
		GPIO_MCUX_LPC_IRQ(n, 3);						\
											\
		return 0;								\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_MCUX_LPC)
