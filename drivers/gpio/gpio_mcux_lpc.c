/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#define PIN_TO_INPUT_MUX_CONNECTION(port, pin)                                 \
	((PINTSEL0 << PMUX_SHIFT) + (32 * port) + (pin))

#define NO_PINT_INT ((1 << sizeof(pint_pin_int_t)) - 1)

struct gpio_mcux_lpc_config {
	GPIO_Type *gpio_base;
	PINT_Type *pint_base;
	u32_t port_no;
	clock_ip_name_t clock_ip_name;
};

struct gpio_mcux_lpc_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
	/* pin association with PINT id */
	pint_pin_int_t pint_id[32];
	/* interrupt type */
	pint_pin_enable_t int_mode[32];
	/* ISR allocated in device tree to this port */
	u32_t isr_list[8];
	/* index to to table above */
	u32_t isr_list_idx;
};


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
		pint_idx = isr_no - PIN_INT4_IRQn;
	}

	INPUTMUX_AttachSignal(
		INPUTMUX, pint_idx,
		PIN_TO_INPUT_MUX_CONNECTION(port, pin));

	/* Turnoff clock to inputmux to save power. Clock is only needed to make
	 * changes. Can be turned off after.
	 */
	INPUTMUX_Deinit(INPUTMUX);

	return pint_idx;
}

static void gpio_mcux_lpc_port_isr(void *arg);

static int gpio_mcux_lpc_configure(struct device *dev, int access_op, u32_t pin,
				   int flags)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;
	struct gpio_mcux_lpc_data *data = dev->driver_data;
	pint_pin_enable_t interruptMode = kPINT_PinIntEnableNone;

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				interruptMode = kPINT_PinIntEnableRiseEdge;
			} else if (flags & GPIO_INT_DOUBLE_EDGE) {
				interruptMode = kPINT_PinIntEnableBothEdges;
			} else {
				interruptMode = kPINT_PinIntEnableFallEdge;
			}
		} else { /* GPIO_INT_LEVEL */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				interruptMode = kPINT_PinIntEnableHighLevel;
			} else {
				interruptMode = kPINT_PinIntEnableLowLevel;
			}
		}
	} else {
		interruptMode = kPINT_PinIntEnableNone;
	}
	data->int_mode[pin] = interruptMode;

	/* supports access by pin now,you can add access by port when needed */
	if (access_op == GPIO_ACCESS_BY_PIN) {
		u32_t port = config->port_no;

		/* input-0,output-1 */
		if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
			u32_t isr;
			u32_t pint_idx;

			gpio_base->DIR[port] &= ~(BIT(pin));

			isr = get_free_isr(data);
			if (isr == -EINVAL) {
				/* Didn't find any free interrupt allocated to the port */
				return -EINVAL;
			}
			pint_idx = attach_pin_to_isr(port, pin, isr);
			data->pint_id[pin] = pint_idx;

			PINT_PinInterruptConfig(config->pint_base, pint_idx,
				interruptMode, (pint_cb_t)gpio_mcux_lpc_port_isr);
		} else {
			gpio_base->SET[port] = BIT(pin);
			gpio_base->DIR[port] |= BIT(pin);
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int gpio_mcux_lpc_write(struct device *dev, int access_op, u32_t pin,
			       u32_t value)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	/* Check for an invalid pin number */
	if (pin >= ARRAY_SIZE(gpio_base->B[config->port_no])) {
		return -EINVAL;
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		/* Set/Clear the data output for the respective pin */
		gpio_base->B[config->port_no][pin] = value;
	} else { /* return an error for all other options */
		return -EINVAL;
	}

	return 0;
}

static int gpio_mcux_lpc_read(struct device *dev, int access_op, u32_t pin,
			      u32_t *value)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
	GPIO_Type *gpio_base = config->gpio_base;

	*value = gpio_base->PIN[config->port_no];

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = (*value & BIT(pin)) >> pin;
	} else { /* return an error for all other options */
		return -EINVAL;
	}

	return 0;
}

static int gpio_mcux_lpc_manage_cb(struct device *port,
				   struct gpio_callback *callback, bool set)
{
	struct gpio_mcux_lpc_data *data = port->driver_data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_mcux_lpc_enable_cb(struct device *port, int access_op,
				   u32_t pin)
{
	const struct gpio_mcux_lpc_config *config = port->config->config_info;
	struct gpio_mcux_lpc_data *data = port->driver_data;
	u32_t i;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= BIT(pin);
		PINT_EnableCallbackByIndex(config->pint_base,
					   data->pint_id[pin]);
	} else {
		data->pin_callback_enables = 0xFFFFFFFF;
		for (i = 0U; i < 32; i++) {
			if (data->pint_id[i] != NO_PINT_INT) {
				PINT_EnableCallbackByIndex(config->pint_base,
							   data->pint_id[i]);
			}
		}
	}

	return 0;
}

static int gpio_mcux_lpc_disable_cb(struct device *port, int access_op,
				    u32_t pin)
{
	const struct gpio_mcux_lpc_config *config = port->config->config_info;
	struct gpio_mcux_lpc_data *data = port->driver_data;
	u32_t i;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		PINT_DisableCallbackByIndex(config->pint_base,
					    data->pint_id[pin]);
		data->pin_callback_enables &= ~BIT(pin);
	} else {
		for (i = 0U; i < 32; i++) {
			if (data->pint_id[i] != NO_PINT_INT) {
				PINT_DisableCallbackByIndex(config->pint_base,
							    data->pint_id[i]);
			}
		}
		data->pin_callback_enables = 0U;
	}

	return 0;
}

static void gpio_mcux_lpc_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
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

			gpio_fire_callbacks(&data->callbacks, dev, enabled_int);

			PINT_PinInterruptClrStatus(config->pint_base,
						   data->pint_id[pin]);
		}
	}
}

static int gpio_mcux_lpc_init(struct device *dev)
{
	const struct gpio_mcux_lpc_config *config = dev->config->config_info;
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
	.config = gpio_mcux_lpc_configure,
	.write = gpio_mcux_lpc_write,
	.read = gpio_mcux_lpc_read,
	.manage_callback = gpio_mcux_lpc_manage_cb,
	.enable_callback = gpio_mcux_lpc_enable_cb,
	.disable_callback = gpio_mcux_lpc_disable_cb,
};

#ifdef CONFIG_GPIO_MCUX_LPC_PORT0
static int lpc_gpio_0_init(struct device *dev);

static const struct gpio_mcux_lpc_config gpio_mcux_lpc_port0_config = {
	.gpio_base = GPIO,
	.pint_base = PINT, /* TODO: SECPINT issue #16330 */
	.port_no = PORT0_IDX,
	.clock_ip_name = kCLOCK_Gpio0,
};

static struct gpio_mcux_lpc_data gpio_mcux_lpc_port0_data;

DEVICE_AND_API_INIT(gpio_mcux_lpc_port0, DT_INST_0_NXP_LPC_GPIO_LABEL,
		    lpc_gpio_0_init, &gpio_mcux_lpc_port0_data,
		    &gpio_mcux_lpc_port0_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_mcux_lpc_driver_api);

static int lpc_gpio_0_init(struct device *dev)
{
#if defined(DT_INST_0_NXP_LPC_GPIO_IRQ_0) || defined(DT_INST_0_NXP_LPC_GPIO_IRQ_1)
	struct gpio_mcux_lpc_data *data = dev->driver_data;
#endif

	gpio_mcux_lpc_init(dev);

#ifdef DT_INST_0_NXP_LPC_GPIO_IRQ_0
	IRQ_CONNECT(DT_INST_0_NXP_LPC_GPIO_IRQ_0,
			DT_INST_0_NXP_LPC_GPIO_IRQ_0_PRIORITY,
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port0), 0);
	irq_enable(DT_INST_0_NXP_LPC_GPIO_IRQ_0);
	data->isr_list[data->isr_list_idx++] = DT_INST_0_NXP_LPC_GPIO_IRQ_0;
#endif

#ifdef DT_INST_0_NXP_LPC_GPIO_IRQ_1
	IRQ_CONNECT(DT_INST_0_NXP_LPC_GPIO_IRQ_1,
			DT_INST_0_NXP_LPC_GPIO_IRQ_1_PRIORITY,
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port0), 0);
	irq_enable(DT_INST_0_NXP_LPC_GPIO_IRQ_1);
	data->isr_list[data->isr_list_idx++] = DT_INST_0_NXP_LPC_GPIO_IRQ_1;
#endif

	return 0;
}

#endif /* CONFIG_GPIO_MCUX_LPC_PORT0 */

#ifdef CONFIG_GPIO_MCUX_LPC_PORT1
static int lpc_gpio_1_init(struct device *dev);

static const struct gpio_mcux_lpc_config gpio_mcux_lpc_port1_config = {
	.gpio_base = GPIO,
	.pint_base = PINT,
	.port_no = PORT1_IDX,
	.clock_ip_name = kCLOCK_Gpio1,
};

static struct gpio_mcux_lpc_data gpio_mcux_lpc_port1_data;

DEVICE_AND_API_INIT(gpio_mcux_lpc_port1, DT_INST_1_NXP_LPC_GPIO_LABEL,
		    lpc_gpio_1_init, &gpio_mcux_lpc_port1_data,
		    &gpio_mcux_lpc_port1_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_mcux_lpc_driver_api);

static int lpc_gpio_1_init(struct device *dev)
{
#if defined(DT_INST_1_NXP_LPC_GPIO_IRQ_0) || defined(DT_INST_1_NXP_LPC_GPIO_IRQ_1)
	struct gpio_mcux_lpc_data *data = dev->driver_data;
#endif

	gpio_mcux_lpc_init(dev);

#ifdef DT_INST_1_NXP_LPC_GPIO_IRQ_0
	IRQ_CONNECT(DT_INST_1_NXP_LPC_GPIO_IRQ_0,
			DT_INST_1_NXP_LPC_GPIO_IRQ_0_PRIORITY,
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port1), 0);
	irq_enable(DT_INST_1_NXP_LPC_GPIO_IRQ_0);
	data->isr_list[data->isr_list_idx++] = DT_INST_1_NXP_LPC_GPIO_IRQ_0;
#endif

#ifdef DT_INST_1_NXP_LPC_GPIO_IRQ_1
	IRQ_CONNECT(DT_INST_1_NXP_LPC_GPIO_IRQ_1,
			DT_INST_1_NXP_LPC_GPIO_IRQ_1_PRIORITY,
		    gpio_mcux_lpc_port_isr, DEVICE_GET(gpio_mcux_lpc_port1), 0);
	irq_enable(DT_INST_1_NXP_LPC_GPIO_IRQ_1);
	data->isr_list[data->isr_list_idx++] = DT_INST_1_NXP_LPC_GPIO_IRQ_1;
#endif

	return 0;
}

#endif /* CONFIG_GPIO_MCUX_LPC_PORT1 */
