/*
 * Copyright (c) 2022 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numicro_gpio

#include <errno.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/numicro-gpio.h>
#include <NuMicro.h>

#define MODE_PIN_SHIFT(pin)	((pin) * 2)
#define MODE_MASK(pin)		(3 << MODE_PIN_SHIFT(pin))
#define DINOFF_PIN_SHIFT(pin)	((pin) + 16)
#define DINOFF_MASK(pin)	(1 << DINOFF_PIN_SHIFT(pin))
#define PUSEL_PIN_SHIFT(pin)	((pin) * 2)
#define PUSEL_MASK(pin)		(3 << PUSEL_PIN_SHIFT(pin))

#define PORT_PIN_MASK		0xFFFF

struct gpio_numicro_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_T *regs;
};

struct gpio_numicro_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	/*
	 * backup of the INTEN register.
	 * The higher half is RHIEN for whether rising trigger is enabled, and
	 * the lower half is FLIEN for whether falling trigger is enabled.
	 */
	uint32_t interrupt_en_reg_bak;
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
};

static int gpio_numicro_configure(const struct device *dev,
			       gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_numicro_config *cfg = dev->config;
	GPIO_T * const regs = cfg->regs;

	uint32_t mode;
	uint32_t debounce_enable = 0;
	uint32_t schmitt_enable = 0;
	uint32_t disable_input_path = 0;
	uint32_t bias = GPIO_PUSEL_DISABLE;

	/* Pin mode */
	if ((flags & GPIO_OUTPUT) != 0) {
		/* Output */

		if ((flags & GPIO_SINGLE_ENDED) != 0) {
			if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
				mode = GPIO_MODE_OPEN_DRAIN;
			} else {
				/* Output can't be open source */
				return -ENOTSUP;
			}
		} else {
			mode = GPIO_MODE_OUTPUT;
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		/* Input */

		mode = GPIO_MODE_INPUT;

		if ((flags & NUMICRO_GPIO_INPUT_DEBOUNCE) != 0) {
			debounce_enable = 1;
		}

		if ((flags & NUMICRO_GPIO_INPUT_SCHMITT) != 0) {
			schmitt_enable = 1;
		}
	} else {
		/* Deactivated: Analog */

		mode = GPIO_MODE_INPUT;
		disable_input_path = 1;
	}

	/* Bias */
	if ((flags & GPIO_OUTPUT) != 0 || (flags & GPIO_INPUT) != 0) {
		if ((flags & GPIO_PULL_UP) != 0) {
			bias = GPIO_PUSEL_PULL_UP;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			bias = GPIO_PUSEL_PULL_DOWN;
		}
	}

	regs->MODE = (regs->MODE & ~MODE_MASK(pin)) |
		     (mode << MODE_PIN_SHIFT(pin));
	regs->DBEN = (regs->DBEN & ~BIT(pin)) | (debounce_enable << pin);
	regs->SMTEN = (regs->SMTEN & ~BIT(pin)) | (schmitt_enable << pin);
	regs->DINOFF = (regs->DINOFF & ~DINOFF_MASK(pin)) |
		       (disable_input_path << DINOFF_PIN_SHIFT(pin));
	regs->PUSEL = (regs->PUSEL & ~PUSEL_MASK(pin)) |
		      (bias << PUSEL_PIN_SHIFT(pin));

	return 0;
}

static int gpio_numicro_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_numicro_config *cfg = dev->config;

	*value = cfg->regs->PIN & PORT_PIN_MASK;

	return 0;
}

static int gpio_numicro_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_numicro_config *cfg = dev->config;

	cfg->regs->DATMSK = ~mask;
	cfg->regs->DOUT = value;

	return 0;
}

static int gpio_numicro_port_set_bits_raw(const struct device *dev,
				       uint32_t mask)
{
	const struct gpio_numicro_config *cfg = dev->config;

	cfg->regs->DATMSK = ~mask;
	cfg->regs->DOUT = PORT_PIN_MASK;

	return 0;
}

static int gpio_numicro_port_clear_bits_raw(const struct device *dev,
					 uint32_t mask)
{
	const struct gpio_numicro_config *cfg = dev->config;

	cfg->regs->DATMSK = ~mask;
	cfg->regs->DOUT = 0;

	return 0;
}

static int gpio_numicro_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_numicro_config *cfg = dev->config;

	cfg->regs->DATMSK = 0;
	cfg->regs->DOUT ^= mask;

	return 0;
}

static int gpio_numicro_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin, enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_numicro_config *cfg = dev->config;
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	struct gpio_numicro_data *data = dev->data;
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
	uint32_t int_type = 0;
	uint32_t int_level = 0;
	uint32_t int_level_mask = BIT(pin) | BIT(pin + 16);

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY) {
		cfg->regs->INTEN &= ~(BIT(pin) | BIT(pin + 16));
		return 0;
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		cfg->regs->INTEN |= data->interrupt_en_reg_bak & (BIT(pin) | BIT(pin + 16));
		return 0;
	}
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	if (mode != GPIO_INT_MODE_DISABLED) {
		int_type = (mode == GPIO_INT_MODE_LEVEL) ? 1 : 0;

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			int_level = BIT(pin);
			break;
		case GPIO_INT_TRIG_HIGH:
			int_level = BIT(pin + 16);
			break;
		case GPIO_INT_TRIG_BOTH:
			int_level = BIT(pin) | BIT(pin + 16);
			break;
		}
	}

	cfg->regs->INTTYPE = (cfg->regs->INTTYPE & ~BIT(pin)) | (int_type << pin);
	cfg->regs->INTEN = (cfg->regs->INTEN & ~int_level_mask) | int_level;
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	data->interrupt_en_reg_bak = cfg->regs->INTEN;
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	return 0;
}

static int gpio_numicro_manage_callback(const struct device *dev,
					struct gpio_callback *callback,
					bool set)
{
	struct gpio_numicro_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_numicro_isr(const struct device *dev)
{
	const struct gpio_numicro_config *cfg = dev->config;
	struct gpio_numicro_data *data = dev->data;
	uint32_t int_status;

	int_status = cfg->regs->INTSRC;

	/* Clear the port interrupts */
	cfg->regs->INTSRC = int_status;

	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

static const struct gpio_driver_api gpio_numicro_driver_api = {
	.pin_configure = gpio_numicro_configure,
	.port_get_raw = gpio_numicro_port_get_raw,
	.port_set_masked_raw = gpio_numicro_port_set_masked_raw,
	.port_set_bits_raw = gpio_numicro_port_set_bits_raw,
	.port_clear_bits_raw = gpio_numicro_port_clear_bits_raw,
	.port_toggle_bits = gpio_numicro_port_toggle_bits,
	.pin_interrupt_configure = gpio_numicro_pin_interrupt_configure,
	.manage_callback = gpio_numicro_manage_callback,
};

#define GPIO_NUMICRO_INIT(n)						\
	static int gpio_numicro_port##n##_init(const struct device *dev)\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    gpio_numicro_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
		return 0;						\
	}								\
									\
	static struct gpio_numicro_data gpio_numicro_port##n##_data;	\
									\
	static const struct gpio_numicro_config gpio_numicro_port##n##_config = {\
		.common = {						\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},							\
		.regs = (GPIO_T *)DT_INST_REG_ADDR(n),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n,					\
			      gpio_numicro_port##n##_init,		\
			      NULL,					\
			      &gpio_numicro_port##n##_data,		\
			      &gpio_numicro_port##n##_config,		\
			      PRE_KERNEL_1,				\
			      CONFIG_GPIO_INIT_PRIORITY,		\
			      &gpio_numicro_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NUMICRO_INIT)
