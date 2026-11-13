/*
 * Copyright (c) 2023 HPMicro
 * Copyright (c) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hpmicro_hpm_gpio

#include <errno.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <hpm_soc.h>

struct hpm_gpio_config {
	struct gpio_driver_config common;
	GPIO_Type *base;
	uint32_t port;
	void (*irq_config_func)(const struct device *dev);
};

struct hpm_gpio_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int hpm_gpio_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct hpm_gpio_config *config = dev->config;
	uint32_t pin_mask;

	if (pin >= GPIO_MAX_PINS_PER_PORT) {
		return -EINVAL;
	}

	pin_mask = BIT(pin);
	if ((config->common.port_pin_mask & pin_mask) == 0U) {
		return -EINVAL;
	}

	/*
	 * Bias and open-drain/open-source settings are pad properties owned by
	 * pinctrl. Reject them instead of reporting success without applying
	 * the requested configuration.
	 */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN | GPIO_SINGLE_ENDED)) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_MASK) {
		return -EINVAL;
	}

	if ((flags & GPIO_DIR_MASK) == GPIO_DISCONNECTED) {
		/*
		 * The GPIO block can disable the output driver, but the input
		 * buffer is configured by IOC/pinctrl. Do not claim that both
		 * directions are disconnected.
		 */
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			config->base->DO[config->port].SET = pin_mask;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			config->base->DO[config->port].CLEAR = pin_mask;
		}

		config->base->OE[config->port].SET = pin_mask;
	} else {
		config->base->OE[config->port].CLEAR = pin_mask;
	}

	return 0;
}

static int hpm_gpio_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct hpm_gpio_config *config = dev->config;

	*value = config->base->DI[config->port].VALUE;

	return 0;
}

static int hpm_gpio_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct hpm_gpio_config *config = dev->config;

	config->base->DO[config->port].CLEAR = mask & ~value;
	config->base->DO[config->port].SET = mask & value;

	return 0;
}

static int hpm_gpio_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct hpm_gpio_config *config = dev->config;

	config->base->DO[config->port].SET = pins;

	return 0;
}

static int hpm_gpio_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct hpm_gpio_config *config = dev->config;

	config->base->DO[config->port].CLEAR = pins;

	return 0;
}

static int hpm_gpio_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct hpm_gpio_config *config = dev->config;

	config->base->DO[config->port].TOGGLE = pins;

	return 0;
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int hpm_gpio_port_get_direction(const struct device *dev, gpio_port_pins_t map,
				       gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct hpm_gpio_config *config = dev->config;
	uint32_t enabled;

	if (inputs == NULL && outputs == NULL) {
		return -EINVAL;
	}

	enabled = config->base->OE[config->port].VALUE;
	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		*inputs = map & ~enabled;
	}

	if (outputs != NULL) {
		*outputs = map & enabled;
	}

	return 0;
}
#endif

static int hpm_gpio_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					    enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct hpm_gpio_config *config = dev->config;
	uint32_t pin_mask;

	if (pin >= GPIO_MAX_PINS_PER_PORT) {
		return -EINVAL;
	}

	pin_mask = BIT(pin);
	if ((config->common.port_pin_mask & pin_mask) == 0U) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		config->base->IE[config->port].CLEAR = pin_mask;
		config->base->IF[config->port].VALUE = pin_mask;
		return 0;
	}

	if (config->irq_config_func == NULL) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		config->base->TP[config->port].CLEAR = pin_mask;

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			config->base->PL[config->port].SET = pin_mask;
			break;
		case GPIO_INT_TRIG_HIGH:
			config->base->PL[config->port].CLEAR = pin_mask;
			break;
		default:
			return -ENOTSUP;
		}
	} else if (mode == GPIO_INT_MODE_EDGE) {
#if defined(GPIO_SOC_HAS_EDGE_BOTH_INTERRUPT) && (GPIO_SOC_HAS_EDGE_BOTH_INTERRUPT == 1)
		config->base->PD[config->port].CLEAR = pin_mask;
#else
		if (trig == GPIO_INT_TRIG_BOTH) {
			return -ENOTSUP;
		}
#endif
		config->base->TP[config->port].SET = pin_mask;

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			config->base->PL[config->port].SET = pin_mask;
			break;
		case GPIO_INT_TRIG_HIGH:
			config->base->PL[config->port].CLEAR = pin_mask;
			break;
		case GPIO_INT_TRIG_BOTH:
#if defined(GPIO_SOC_HAS_EDGE_BOTH_INTERRUPT) && (GPIO_SOC_HAS_EDGE_BOTH_INTERRUPT == 1)
			config->base->PD[config->port].SET = pin_mask;
			break;
#else
			return -ENOTSUP;
#endif
		default:
			return -ENOTSUP;
		}
	} else {
		return -ENOTSUP;
	}

	config->base->IE[config->port].CLEAR = pin_mask;
	config->base->IF[config->port].VALUE = pin_mask;
	config->base->IE[config->port].SET = pin_mask;

	return 0;
}

static int hpm_gpio_manage_callback(const struct device *dev, struct gpio_callback *callback,
				    bool set)
{
	struct hpm_gpio_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts)
static void hpm_gpio_isr(const struct device *dev)
{
	const struct hpm_gpio_config *config = dev->config;
	struct hpm_gpio_data *data = dev->data;
	uint32_t status;

	status = config->base->IF[config->port].VALUE;
	status &= config->base->IE[config->port].VALUE & config->common.port_pin_mask;
	config->base->IF[config->port].VALUE = status;

	gpio_fire_callbacks(&data->callbacks, dev, status);
}
#endif

static DEVICE_API(gpio, hpm_gpio_api) = {
	.pin_configure = hpm_gpio_pin_configure,
	.port_get_raw = hpm_gpio_port_get_raw,
	.port_set_masked_raw = hpm_gpio_port_set_masked_raw,
	.port_set_bits_raw = hpm_gpio_port_set_bits_raw,
	.port_clear_bits_raw = hpm_gpio_port_clear_bits_raw,
	.port_toggle_bits = hpm_gpio_port_toggle_bits,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = hpm_gpio_port_get_direction,
#endif
	.pin_interrupt_configure = hpm_gpio_pin_interrupt_configure,
	.manage_callback = hpm_gpio_manage_callback,
};

#define HPM_GPIO_IRQ_CONFIG(n)                                                                     \
	static void hpm_gpio_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), hpm_gpio_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define HPM_GPIO_IRQ_CONFIG_OR_NONE(n)                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts), (HPM_GPIO_IRQ_CONFIG(n)), ())

#define HPM_GPIO_IRQ_CONFIG_FIELD(n)                                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, interrupts),                                          \
		    (.irq_config_func = hpm_gpio_irq_config_##n,), (.irq_config_func = NULL,))

#define HPM_GPIO_INIT(n)                                                                           \
	HPM_GPIO_IRQ_CONFIG_OR_NONE(n)                                                             \
                                                                                                   \
	static struct hpm_gpio_data hpm_gpio_data_##n;                                             \
                                                                                                   \
	static const struct hpm_gpio_config hpm_gpio_config_##n = {                                \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = (GPIO_Type *)DT_REG_ADDR(DT_INST_PARENT(n)),                               \
		.port = DT_INST_REG_ADDR(n),                                                       \
		HPM_GPIO_IRQ_CONFIG_FIELD(n)};                                                     \
                                                                                                   \
	static int hpm_gpio_init_##n(const struct device *dev)                                     \
	{                                                                                          \
		const struct hpm_gpio_config *config = dev->config;                                \
                                                                                                   \
		config->base->IE[config->port].VALUE = 0U;                                         \
		config->base->IF[config->port].VALUE = config->common.port_pin_mask;               \
                                                                                                   \
		if (config->irq_config_func != NULL) {                                             \
			config->irq_config_func(dev);                                              \
		}                                                                                  \
                                                                                                   \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, hpm_gpio_init_##n, NULL, &hpm_gpio_data_##n,                      \
			      &hpm_gpio_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,       \
			      &hpm_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(HPM_GPIO_INIT)
