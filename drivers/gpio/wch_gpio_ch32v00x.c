/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/wch_exti.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>

#include <hal_ch32fun.h>

#define DT_DRV_COMPAT wch_gpio

struct gpio_ch32v00x_config {
	struct gpio_driver_config common;
	GPIO_TypeDef *regs;
	const struct device *clock_dev;
	uint8_t clock_id;
};

struct gpio_ch32v00x_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int gpio_ch32v00x_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ch32v00x_config *config = dev->config;
	GPIO_TypeDef *regs = config->regs;
	uint32_t cnf_mode;
	uint32_t bshr = 0;

	if ((flags & GPIO_OUTPUT) != 0) {
		cnf_mode = 0x01;
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			bshr = 1 << pin;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			bshr = 1 << (16 + pin);
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		if ((flags & GPIO_PULL_UP) != 0) {
			cnf_mode = GPIO_CFGLR_IN_PUPD;
			bshr = 1 << pin;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			cnf_mode = GPIO_CFGLR_IN_PUPD;
			bshr = 1 << (16 + pin);
		} else {
			cnf_mode = GPIO_CFGLR_IN_FLOAT;
		}
	} else {
		cnf_mode = 0x00;
	}

	if (pin < 8) {
		regs->CFGLR = (regs->CFGLR & ~(0x0F << (4 * pin))) | (cnf_mode << (4 * pin));
	} else {
		regs->CFGHR =
			(regs->CFGHR & ~(0x0F << ((pin - 8) * 4))) | (cnf_mode << ((pin - 8) * 4));
	}
	regs->BSHR = bshr;

	return 0;
}

static int gpio_ch32v00x_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	*value = config->regs->INDR;

	return 0;
}

static int gpio_ch32v00x_port_set_masked_raw(const struct device *dev, uint32_t mask,
					     uint32_t value)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	config->regs->BSHR = ((~value & mask) << 16) | (value & mask);

	return 0;
}

static int gpio_ch32v00x_port_set_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	config->regs->BSHR = pins;

	return 0;
}

static int gpio_ch32v00x_port_clear_bits_raw(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	config->regs->BCR = pins;

	return 0;
}

static int gpio_ch32v00x_port_toggle_bits(const struct device *dev, uint32_t pins)
{
	const struct gpio_ch32v00x_config *config = dev->config;
	uint32_t current = config->regs->OUTDR;

	config->regs->BSHR = (~current & pins) | (current & pins) << 16;

	return 0;
}

#if defined(CONFIG_GPIO_WCH_GPIO_INTERRUPTS)

static void gpio_ch32v00x_isr(uint8_t line, void *user)
{
	const struct device *dev = user;
	struct gpio_ch32v00x_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(line));
}

static int gpio_ch32v00x_configure_exti(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_ch32v00x_config *config = dev->config;
	AFIO_TypeDef *afio = (AFIO_TypeDef *)DT_REG_ADDR(DT_NODELABEL(pinctrl));
	uint8_t port_id;
	uint8_t cr_id;
	uint8_t bit0;

	/* Convert the device into a port ID by checking the address */
	switch ((uintptr_t)config->regs) {
	case DT_REG_ADDR(DT_NODELABEL(gpioa)):
		port_id = 0;
		break;
#if DT_NODE_EXISTS(DT_NODELABEL(gpiob))
	case DT_REG_ADDR(DT_NODELABEL(gpiob)):
		port_id = 1;
		break;
#endif
	case DT_REG_ADDR(DT_NODELABEL(gpioc)):
		port_id = 2;
		break;
	case DT_REG_ADDR(DT_NODELABEL(gpiod)):
		port_id = 3;
		break;
#if DT_NODE_EXISTS(DT_NODELABEL(gpioe))
	case DT_REG_ADDR(DT_NODELABEL(gpioe)):
		port_id = 4;
		break;
#endif
	default:
		return -EINVAL;
	}

#if defined(AFIO_EXTICR_EXTI0)
	/* CH32V003 style with one register with 2 bits per map. */
	BUILD_ASSERT(AFIO_EXTICR_EXTI0 == 0x03);

	(void)cr_id;
	bit0 = pin << 1;
	afio->EXTICR = (afio->EXTICR & ~(AFIO_EXTICR_EXTI0 << bit0)) | (port_id << bit0);
#elif defined(AFIO_EXTICR1_EXTI0)
	/*
	 * CH32V20x style with multiple registers with 4 pins per register and 4 bits per
	 * map.
	 */
	BUILD_ASSERT(AFIO_EXTICR1_EXTI0 == 0x0F);
	BUILD_ASSERT(ARRAY_SIZE(afio->EXTICR) == 4);

	cr_id = pin / 4;
	bit0 = (pin % 4) * 4;
	afio->EXTICR[cr_id] =
		(afio->EXTICR[cr_id] & ~(AFIO_EXTICR1_EXTI0 << bit0)) | (port_id << bit0);
#else
#error Unrecognised EXTICR format
#endif

	return 0;
}

static int gpio_ch32v00x_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trigger)
{
	int err;

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		wch_exti_disable(pin);
		err = wch_exti_configure(pin, NULL, NULL);
		break;
	case GPIO_INT_MODE_EDGE:
		err = wch_exti_configure(pin, gpio_ch32v00x_isr, (void *)dev);
		if (err != 0) {
			break;
		}

		err = gpio_ch32v00x_configure_exti(dev, pin);
		if (err != 0) {
			break;
		}

		switch (trigger) {
		case GPIO_INT_TRIG_LOW:
			wch_exti_set_trigger(pin, WCH_EXTI_TRIGGER_FALLING_EDGE);
			break;
		case GPIO_INT_TRIG_HIGH:
			wch_exti_set_trigger(pin, WCH_EXTI_TRIGGER_RISING_EDGE);
			break;
		case GPIO_INT_TRIG_BOTH:
			wch_exti_set_trigger(pin, WCH_EXTI_TRIGGER_FALLING_EDGE |
							  WCH_EXTI_TRIGGER_RISING_EDGE);
			break;
		default:
			return -ENOTSUP;
		}

		wch_exti_enable(pin);
		break;
	default:
		return -ENOTSUP;
	}

	return err;
}

static int gpio_ch32v00x_manage_callback(const struct device *dev, struct gpio_callback *callback,
					 bool set)
{
	struct gpio_ch32v00x_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

#endif /* CONFIG_GPIO_WCH_GPIO_INTERRUPTS */

static DEVICE_API(gpio, gpio_ch32v00x_driver_api) = {
	.pin_configure = gpio_ch32v00x_configure,
	.port_get_raw = gpio_ch32v00x_port_get_raw,
	.port_set_masked_raw = gpio_ch32v00x_port_set_masked_raw,
	.port_set_bits_raw = gpio_ch32v00x_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ch32v00x_port_clear_bits_raw,
	.port_toggle_bits = gpio_ch32v00x_port_toggle_bits,
#if defined(CONFIG_GPIO_WCH_GPIO_INTERRUPTS)
	.pin_interrupt_configure = gpio_ch32v00x_pin_interrupt_configure,
	.manage_callback = gpio_ch32v00x_manage_callback,
#endif
};

static int gpio_ch32v00x_init(const struct device *dev)
{
	const struct gpio_ch32v00x_config *config = dev->config;

	clock_control_on(config->clock_dev, (clock_control_subsys_t *)(uintptr_t)config->clock_id);

	return 0;
}

#define GPIO_CH32V00X_INIT(idx)                                                                    \
	static const struct gpio_ch32v00x_config gpio_ch32v00x_##idx##_config = {                  \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),             \
			},                                                                         \
		.regs = (GPIO_TypeDef *)DT_INST_REG_ADDR(idx),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_id = DT_INST_CLOCKS_CELL(idx, id),                                          \
	};                                                                                         \
                                                                                                   \
	static struct gpio_ch32v00x_data gpio_ch32v00x_##idx##_data;                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, gpio_ch32v00x_init, NULL, &gpio_ch32v00x_##idx##_data,          \
			      &gpio_ch32v00x_##idx##_config, PRE_KERNEL_1,                         \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_ch32v00x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CH32V00X_INIT)
