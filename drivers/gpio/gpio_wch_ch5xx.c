/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>

#include <hal_ch32fun.h>

#define DT_DRV_COMPAT wch_ch5xx_gpio

/* Check if an instance has a specific enum value */
#define DT_ANY_INST_HAS_PROP_ENUM_VALUE(inst, prop, value)                                         \
	IF_ENABLED(DT_INST_ENUM_HAS_VALUE(inst, prop, value), (1,))

/* Check if any instance has status okay and matches the port B property */
#define HAS_GPIO_PORTB_STATUS_OKAY                                                                 \
	UTIL_NOT(IS_EMPTY(                                                                         \
		DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_ANY_INST_HAS_PROP_ENUM_VALUE, port, port_b)))

/* Enable port B support if available in the device tree */
#if HAS_GPIO_PORTB_STATUS_OKAY
#define GPIO_REG(bits, port, reg)                                                                  \
	(*(volatile uint##bits##_t *)(port == 0 ? &regs->PA_##reg : &regs->PB_##reg))
#else
#define GPIO_REG(bits, port, reg) (regs->PA_##reg)
#endif

struct gpio_ch5xx_config {
	struct gpio_driver_config common;
	GPIO_TypeDef *regs;
	uint8_t port; /* 0 for port A, 1 for port B */
};

struct gpio_ch5xx_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int gpio_ch5xx_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;
	uint32_t pin_mask = BIT(pin);

	if ((flags & GPIO_OUTPUT) != 0) {
		GPIO_REG(32, config->port, DIR) |= pin_mask;
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			GPIO_REG(32, config->port, SET) = pin_mask;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			GPIO_REG(32, config->port, CLR) = pin_mask;
		} else {
			/* No output level specified */
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		GPIO_REG(32, config->port, DIR) &= ~pin_mask;
		if ((flags & GPIO_PULL_UP) != 0) {
			GPIO_REG(32, config->port, PU) |= pin_mask;
			GPIO_REG(32, config->port, PD_DRV) &= ~pin_mask;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			GPIO_REG(32, config->port, PU) &= ~pin_mask;
			GPIO_REG(32, config->port, PD_DRV) |= pin_mask;
		} else { /* floating input */
			GPIO_REG(32, config->port, PU) &= ~pin_mask;
			GPIO_REG(32, config->port, PD_DRV) &= ~pin_mask;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_ch5xx_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;

	*value = GPIO_REG(32, config->port, PIN);

	return 0;
}

static int gpio_ch5xx_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;

	GPIO_REG(32, config->port, SET) = value & mask;
	GPIO_REG(32, config->port, CLR) = (~value) & mask;

	return 0;
}

static int gpio_ch5xx_port_set_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;

	GPIO_REG(32, config->port, SET) = pins;

	return 0;
}

static int gpio_ch5xx_port_clear_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;

	GPIO_REG(32, config->port, CLR) = pins;

	return 0;
}

static int gpio_ch5xx_port_toggle_bits(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;

	GPIO_REG(32, config->port, OUT) ^= pins;

	return 0;
}

#if defined(CONFIG_GPIO_GET_DIRECTION)
static int gpio_ch5xx_port_get_direction(const struct device *dev, uint32_t map, uint32_t *inputs,
					 uint32_t *outputs)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;
	uint32_t dir = GPIO_REG(32, config->port, DIR);

	if (inputs) {
		*inputs = ~dir & map;
	}
	if (outputs) {
		*outputs = dir & map;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

#if defined(CONFIG_GPIO_WCH_CH5XX_INTERRUPTS)
static void gpio_ch5xx_isr(const struct device *dev)
{
	struct gpio_ch5xx_data *data = dev->data;
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;

	uint16_t pending = GPIO_REG(16, config->port, INT_IF) & GPIO_REG(16, config->port, INT_EN);

	GPIO_REG(16, config->port, INT_IF) = pending;

	gpio_fire_callbacks(&data->callbacks, dev, (uint32_t)pending);
}

static int gpio_ch5xx_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trigger)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;
	uint16_t pin_mask = BIT(pin);

	GPIO_REG(16, config->port, INT_IF) = pin_mask;

	if (mode == GPIO_INT_MODE_DISABLED) {
		GPIO_REG(16, config->port, INT_EN) &= ~pin_mask;
		return 0;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		GPIO_REG(16, config->port, INT_MODE) &= ~pin_mask;
	} else if (mode == GPIO_INT_MODE_EDGE) {
		GPIO_REG(16, config->port, INT_MODE) |= pin_mask;
	} else {
		return -ENOTSUP;
	}

	switch (trigger) {
	case GPIO_INT_TRIG_LOW:
		GPIO_REG(16, config->port, INT_EDGE_TYPE) &= ~pin_mask;
		GPIO_REG(16, config->port, OUT) &= ~pin_mask;
		break;
	case GPIO_INT_TRIG_HIGH:
		GPIO_REG(16, config->port, INT_EDGE_TYPE) &= ~pin_mask;
		GPIO_REG(16, config->port, OUT) |= pin_mask;
		break;
	case GPIO_INT_TRIG_BOTH:
		GPIO_REG(16, config->port, INT_EDGE_TYPE) |= pin_mask;
		break;
	default:
		return -ENOTSUP;
	}

	GPIO_REG(16, config->port, INT_EN) |= pin_mask;

	return 0;
}

static int gpio_ch5xx_manage_callback(const struct device *dev, struct gpio_callback *callback,
				      bool set)
{
	struct gpio_ch5xx_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_ch5xx_get_pending_int(const struct device *dev)
{
	const struct gpio_ch5xx_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;

	return GPIO_REG(16, config->port, INT_IF) & GPIO_REG(16, config->port, INT_EN);
}

#endif /* CONFIG_GPIO_WCH_GPIO_INTERRUPTS */

static DEVICE_API(gpio, gpio_ch5xx_driver_api) = {
	.pin_configure = gpio_ch5xx_configure,
	.port_get_raw = gpio_ch5xx_port_get_raw,
	.port_set_masked_raw = gpio_ch5xx_port_set_masked_raw,
	.port_set_bits_raw = gpio_ch5xx_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ch5xx_port_clear_bits_raw,
	.port_toggle_bits = gpio_ch5xx_port_toggle_bits,
#if defined(CONFIG_GPIO_GET_DIRECTION)
	.port_get_direction = gpio_ch5xx_port_get_direction,
#endif
#if defined(CONFIG_GPIO_WCH_CH5XX_INTERRUPTS)
	.pin_interrupt_configure = gpio_ch5xx_pin_interrupt_configure,
	.manage_callback = gpio_ch5xx_manage_callback,
	.get_pending_int = gpio_ch5xx_get_pending_int,
#endif
};

#if defined(CONFIG_GPIO_WCH_CH5XX_INTERRUPTS)
#define GPIO_CH5XX_IRQ_INIT(idx)                                                                   \
	static int gpio_ch5xx_##idx##_irq_init(const struct device *dev)                           \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(idx), DT_INST_IRQ(idx, priority), gpio_ch5xx_isr,         \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQN(idx));                                                     \
		return 0;                                                                          \
	}
#else
#define GPIO_CH5XX_IRQ_INIT(idx)
#endif

#define GPIO_CH5XX_IRQ_INIT_PTR(idx)                                                               \
	COND_CODE_1(CONFIG_GPIO_WCH_CH5XX_INTERRUPTS, (gpio_ch5xx_##idx##_irq_init), (NULL))
#define GPIO_CH5XX_INIT(idx)                                                                       \
	static const struct gpio_ch5xx_config gpio_ch5xx_##idx##_config = {                        \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(idx),                                    \
		.regs = (GPIO_TypeDef *)DT_INST_REG_ADDR(idx),                                     \
		.port = DT_INST_ENUM_IDX(idx, port),                                               \
	};                                                                                         \
	static struct gpio_ch5xx_data gpio_ch5xx_##idx##_data;                                     \
	GPIO_CH5XX_IRQ_INIT(idx)                                                                   \
	DEVICE_DT_INST_DEFINE(idx, GPIO_CH5XX_IRQ_INIT_PTR(idx), NULL, &gpio_ch5xx_##idx##_data,   \
			      &gpio_ch5xx_##idx##_config, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
			      &gpio_ch5xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CH5XX_INIT)
