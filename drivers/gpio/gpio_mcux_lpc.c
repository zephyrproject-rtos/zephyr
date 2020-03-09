/*
 * Copyright (c) 2017, NXP
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
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include "gpio_utils.h"
#include <fsl_gpio.h>
#include <fsl_pint.h>
#include <fsl_inputmux.h>
#include <fsl_device_registers.h>

#define PORT0_IDX 0u
#define PORT1_IDX 1u

#define PIN_TO_INPUT_MUX_CONNECTION(port, pin) \
	((PINTSEL0 << PMUX_SHIFT) + (32 * port) + (pin))

#define NO_PINT_INT ((1 << sizeof(pint_pin_int_t)) - 1)

struct gpio_mcux_lpc_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *gpio_base;
	PINT_Type *pint_base;
	IOCON_Type *pinmux_base;
	u32_t port_no;
	clock_ip_name_t clock_ip_name;
};

struct gpio_mcux_lpc_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
	/* pin association with PINT id */
	pint_pin_int_t pint_id[32];
	/* ISR allocated in device tree to this port */
	u32_t isr_list[8];
	/* index to to table above */
	u32_t isr_list_idx;
};

static int gpio_mcux_lpc_configure(struct device *dev, gpio_pin_t pin,
				   gpio_flags_t flags)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	GPIO_Type *gpio_base = config->gpio_base;
	u32_t port = config->port_no;

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		IOCON_Type *pinmux_base = config->pinmux_base;
		u32_t *pinconfig = (u32_t *)&(pinmux_base->PIO[port][pin]);

		*pinconfig &= ~(IOCON_PIO_MODE_PULLUP|IOCON_PIO_MODE_PULLDOWN);
		if ((flags & GPIO_PULL_UP) != 0) {
			*pinconfig |= IOCON_PIO_MODE_PULLUP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			*pinconfig |= IOCON_PIO_MODE_PULLDOWN;
		}
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

static int gpio_mcux_lpc_port_get_raw(struct device *dev, u32_t *value)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PIN[config->port_no];

	return 0;
}

static int gpio_mcux_lpc_port_set_masked_raw(struct device *dev, u32_t mask,
					     u32_t value)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	GPIO_Type *gpio_base = config->gpio_base;
	u32_t port = config->port_no;

	/* Writing 0 allows R+W, 1 disables the pin */
	gpio_base->MASK[port] = ~mask;
	gpio_base->PIN[port] = value;
	/* Enable back the pins, user won't assume pins remain masked*/
	gpio_base->MASK[port] = 0U;

	return 0;
}

static int gpio_mcux_lpc_port_set_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->SET[config->port_no] = mask;

	return 0;
}

static int gpio_mcux_lpc_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->CLR[config->port_no] = mask;

	return 0;
}

static int gpio_mcux_lpc_port_toggle_bits(struct device *dev, u32_t mask)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	gpio_base->NOT[config->port_no] = mask;

	return 0;
}

static void gpio_mcux_lpc_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	struct gpio_mcux_lpc_data *data = dev->driver_data;
	u32_t enabled_int;
	u32_t int_flags;
	u32_t pin;

	for (pin = 0; pin < 32; pin++) {
		if (data->pint_id[pin] != NO_PINT_INT) {
			int_flags = PINT_PinInterruptGetStatus(
				config->pint_base, data->pint_id[pin]);
			enabled_int =
				(int_flags << pin) & data->pin_callback_enables;

			PINT_PinInterruptClrStatus(config->pint_base,
						   data->pint_id[pin]);

			gpio_fire_callbacks(&data->callbacks, dev, enabled_int);
		}
	}
}

static u32_t get_free_isr(struct gpio_mcux_lpc_data *data)
{
	u32_t i;
	u32_t isr;

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
static u32_t attach_pin_to_isr(u32_t port, u32_t pin, u32_t isr_no)
{
	u32_t pint_idx;
	/* Connect trigger sources to PINT */
	INPUTMUX_Init(INPUTMUX);

	/* Code asumes PIN_INT values are grouped [0..3] and [4..7].
	 * This scenario is true in LPC54xxx/LPC55xxx.
	 */
	if (isr_no < PIN_INT4_IRQn) {
		pint_idx = isr_no - PIN_INT0_IRQn;
	} else {
		pint_idx = isr_no - PIN_INT4_IRQn + 4;
	}

	INPUTMUX_AttachSignal(INPUTMUX, pint_idx,
			      PIN_TO_INPUT_MUX_CONNECTION(port, pin));

	/* Turnoff clock to inputmux to save power. Clock is only needed to make
	 * changes. Can be turned off after.
	 */
	INPUTMUX_Deinit(INPUTMUX);

	return pint_idx;
}

static void gpio_mcux_lpc_port_isr(void *arg);


static int gpio_mcux_lpc_pin_interrupt_configure(struct device *dev,
		gpio_pin_t pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	struct gpio_mcux_lpc_data *data = dev->driver_data;
	pint_pin_enable_t interruptMode = kPINT_PinIntEnableNone;
	GPIO_Type *gpio_base = config->gpio_base;
	u32_t port = config->port_no;
	u32_t isr;
	u32_t pint_idx;

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

	PINT_PinInterruptConfig(config->pint_base, data->pint_id[pin],
		interruptMode,
		(pint_cb_t)gpio_mcux_lpc_port_isr);

	WRITE_BIT(data->pin_callback_enables, pin, mode != GPIO_INT_DISABLE);

	return 0;
}

static int gpio_mcux_lpc_manage_cb(struct device *port,
				   struct gpio_callback *callback, bool set)
{
	struct gpio_mcux_lpc_data *data = port->driver_data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_mcux_lpc_enable_cb(struct device *port,
				   gpio_pin_t pin)
{
	struct gpio_mcux_lpc_data *data = port->driver_data;

	data->pin_callback_enables |= BIT(pin);

	return 0;
}

static int gpio_mcux_lpc_disable_cb(struct device *port,
				    gpio_pin_t pin)
{
	struct gpio_mcux_lpc_data *data = port->driver_data;

	data->pin_callback_enables &= ~BIT(pin);

	return 0;
}

static int gpio_mcux_lpc_init(struct device *dev)
{
	const struct gpio_mcux_lpc_config *config = dev->config_info;
	struct gpio_mcux_lpc_data *data = dev->driver_data;
	int i;

	CLOCK_EnableClock(config->clock_ip_name);

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
	.enable_callback = gpio_mcux_lpc_enable_cb,
	.disable_callback = gpio_mcux_lpc_disable_cb,
};

#ifdef CONFIG_GPIO_MCUX_LPC_PORT0
static int lpc_gpio_0_init(struct device *dev);

static const struct gpio_mcux_lpc_config gpio_mcux_lpc_port0_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
	},
	.gpio_base = GPIO,
	.pint_base = PINT, /* TODO: SECPINT issue #16330 */
	.pinmux_base = IOCON,
	.port_no = PORT0_IDX,
	.clock_ip_name = kCLOCK_Gpio0,
};

static struct gpio_mcux_lpc_data gpio_mcux_lpc_port0_data;

DEVICE_AND_API_INIT(gpio_mcux_lpc_port0, DT_INST_LABEL(0),
		    lpc_gpio_0_init, &gpio_mcux_lpc_port0_data,
		    &gpio_mcux_lpc_port0_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_lpc_driver_api);

static int lpc_gpio_0_init(struct device *dev)
{
#if DT_INST_IRQ_HAS_IDX(0, 0) || \
	DT_INST_IRQ_HAS_IDX(0, 1) || \
	DT_INST_IRQ_HAS_IDX(0, 2) || \
	DT_INST_IRQ_HAS_IDX(0, 3)
	struct gpio_mcux_lpc_data *data = dev->driver_data;
#endif

	gpio_mcux_lpc_init(dev);

#if DT_INST_IRQ_HAS_IDX(0, 0)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 0, irq),
		    DT_INST_IRQ_BY_IDX(0, 0, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 0, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(0, 0, irq);
#endif

#if DT_INST_IRQ_HAS_IDX(0, 1)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 1, irq),
		    DT_INST_IRQ_BY_IDX(0, 1, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 1, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(0, 1, irq);
#endif

#if DT_INST_IRQ_HAS_IDX(0, 2)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 2, irq),
		    DT_INST_IRQ_BY_IDX(0, 2, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 2, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(0, 2, irq);
#endif

#if DT_INST_IRQ_HAS_IDX(0, 3)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 3, irq),
		    DT_INST_IRQ_BY_IDX(0, 3, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 3, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(0, 3, irq);
#endif

	return 0;
}

#endif /* CONFIG_GPIO_MCUX_LPC_PORT0 */

#ifdef CONFIG_GPIO_MCUX_LPC_PORT1
static int lpc_gpio_1_init(struct device *dev);

static const struct gpio_mcux_lpc_config gpio_mcux_lpc_port1_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(1),
	},
	.gpio_base = GPIO,
	.pint_base = PINT,
	.pinmux_base = IOCON,
	.port_no = PORT1_IDX,
	.clock_ip_name = kCLOCK_Gpio1,
};

static struct gpio_mcux_lpc_data gpio_mcux_lpc_port1_data;

DEVICE_AND_API_INIT(gpio_mcux_lpc_port1, DT_INST_LABEL(1),
		    lpc_gpio_1_init, &gpio_mcux_lpc_port1_data,
		    &gpio_mcux_lpc_port1_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_mcux_lpc_driver_api);

static int lpc_gpio_1_init(struct device *dev)
{
#if DT_INST_IRQ_HAS_IDX(1, 0) || \
	DT_INST_IRQ_HAS_IDX(1, 1) || \
	DT_INST_IRQ_HAS_IDX(1, 2) || \
	DT_INST_IRQ_HAS_IDX(1, 3)
	struct gpio_mcux_lpc_data *data = dev->driver_data;
#endif

	gpio_mcux_lpc_init(dev);

#if DT_INST_IRQ_HAS_IDX(1, 0)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(1, 0, irq),
		    DT_INST_IRQ_BY_IDX(1, 0, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port1), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(1, 0, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(1, 0, irq);
#endif

#if DT_INST_IRQ_HAS_IDX(1, 1)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(1, 1, irq),
		    DT_INST_IRQ_BY_IDX(1, 1, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port1), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(1, 1, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(1, 1, irq);
#endif

#if DT_INST_IRQ_HAS_IDX(1, 2)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(1, 2, irq),
		    DT_INST_IRQ_BY_IDX(1, 2, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port1), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(1, 2, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(1, 2, irq);
#endif

#if DT_INST_IRQ_HAS_IDX(1, 3)
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(1, 3, irq),
		    DT_INST_IRQ_BY_IDX(1, 3, priority),
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port1), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(1, 3, irq));
	data->isr_list[data->isr_list_idx++] = DT_INST_IRQ_BY_IDX(1, 3, irq);
#endif

	return 0;
}

#endif /* CONFIG_GPIO_MCUX_LPC_PORT1 */
