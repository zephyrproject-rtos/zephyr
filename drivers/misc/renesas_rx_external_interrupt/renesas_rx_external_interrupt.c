/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#define DT_DRV_COMPAT renesas_rx_external_interrupt

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/misc/renesas_rx_external_interrupt/renesas_rx_external_interrupt.h>
#include <zephyr/drivers/interrupt_controller/intc_rx_icu.h>
#include <soc.h>

struct gpio_rx_irq_config {
	mem_addr_t reg;
	unsigned int channel;
	enum icu_irq_mode trigger;
	uint8_t sample_clock;
	enum icu_dig_filt digital_filter;
	unsigned int irq;
};

struct gpio_rx_irq_data {
	struct gpio_rx_callback callback;
};

/**
 * @brief setting interrupt for gpio input
 *
 * @param dev devive instance for gpio interrupt line
 * @param callback setting context for the callback
 * @retval 0 if success
 * @retval -EBUSY if interrupt line is inuse
 * @retval -ENOTSUP if interrupt mode is not supported
 */
int gpio_rx_interrupt_set(const struct device *dev, struct gpio_rx_callback *callback)
{
	const struct gpio_rx_irq_config *config = dev->config;
	struct gpio_rx_irq_data *data = dev->data;
	enum icu_irq_mode trigger;
	int ret = 0;

	if (callback->mode == GPIO_INT_MODE_LEVEL) {
		if (callback->trigger != GPIO_INT_TRIG_LOW) {
			return -ENOTSUP;
		}

		trigger = ICU_LOW_LEVEL;
	} else if (callback->mode == GPIO_INT_MODE_EDGE) {
		switch (callback->trigger) {
		case GPIO_INT_TRIG_LOW:
			trigger = ICU_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			trigger = ICU_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			trigger = ICU_BOTH_EDGE;
			break;
		default:
			return -ENOTSUP;
		}
	} else {
		return -ENOTSUP;
	}

	ret = rx_icu_set_irq_control(config->channel, trigger);
	if (ret < 0) {
		return -ENOTSUP;
	}
	data->callback = *callback;
	irq_enable(config->irq);

	return 0;
}

/**
 * @brief unset interrupt configuration for the gpio interrupt
 *
 * @param dev device instance for port irq line
 * @param port_num gpio port number
 * @param pin the pin to disable interrupt
 */
void gpio_rx_interrupt_unset(const struct device *dev, uint8_t port_num, uint8_t pin)
{
	const struct gpio_rx_irq_config *config = dev->config;
	struct gpio_rx_irq_data *data = dev->data;

	if ((port_num != data->callback.port_num) && (pin != data->callback.pin)) {
		return;
	}

	irq_disable(config->irq);
}

void gpio_rx_isr(const struct device *dev)
{
	const struct gpio_rx_irq_data *data = dev->data;
	const struct gpio_rx_irq_config *config = dev->config;

	rx_icu_clear_ir_flag(config->irq);
	data->callback.isr(data->callback.port, data->callback.pin);
}

static int gpio_rx_interrupt_init(const struct device *dev)
{
	const struct gpio_rx_irq_config *config = dev->config;

	rx_irq_dig_filt_t dig_filt = {
		.filt_clk_div = config->sample_clock,
		.filt_enable = config->digital_filter,
	};

	if (config->digital_filter) {
		rx_icu_set_irq_dig_filt(config->channel, dig_filt);
	}

	return 0;
}

#define GPIO_INTERRUPT_INIT(index)                                                                 \
	static const struct gpio_rx_irq_config gpio_rx_irq_config##index = {                       \
		.reg = DT_INST_REG_ADDR(index),                                                    \
		.channel = DT_INST_PROP(index, channel),                                           \
		.trigger = DT_INST_ENUM_IDX_OR(index, renesas_trigger, ICU_FALLING),               \
		.digital_filter = DT_INST_PROP_OR(index, renesas_digital_filtering, false),        \
		.irq = DT_INST_IRQ(index, irq),                                                    \
	};                                                                                         \
	static struct gpio_rx_irq_data gpio_rx_irq_data##index;                                    \
	static int gpio_rx_irq_init##index(const struct device *dev)                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(index, irq), DT_INST_IRQ(index, priority), gpio_rx_isr,    \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		return gpio_rx_interrupt_init(dev);                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, gpio_rx_irq_init##index, NULL, &gpio_rx_irq_data##index,      \
			      &gpio_rx_irq_config##index, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_INTERRUPT_INIT)
