/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_external_interrupt

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/misc/renesas_ra_external_interrupt/renesas_ra_external_interrupt.h>
#include <soc.h>

enum ext_irq_trigger {
	EXT_INTERRUPT_EDGE_FALLING = 0,
	EXT_INTERRUPT_EDGE_RISING,
	EXT_INTERRUPT_EDGE_BOTH,
	EXT_INTERRUPT_EDGE_LOW_LEVEL,
};

enum ext_irq_sample_clock {
	EXT_INTERRUPT_SAMPLE_CLOCK_DIV_1 = 0,
	EXT_INTERRUPT_SAMPLE_CLOCK_DIV_8,
	EXT_INTERRUPT_SAMPLE_CLOCK_DIV_32,
	EXT_INTERRUPT_SAMPLE_CLOCK_DIV_64,
};

struct gpio_ra_irq_config {
	mem_addr_t reg;
	unsigned int channel;
	enum ext_irq_trigger trigger;
	enum ext_irq_sample_clock sample_clock;
	bool digital_filter;
	unsigned int irq;
};

struct gpio_ra_irq_data {
	struct gpio_ra_callback callback;
	struct k_sem irq_sem;
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
int gpio_ra_interrupt_set(const struct device *dev, struct gpio_ra_callback *callback)
{
	const struct gpio_ra_irq_config *config = dev->config;
	struct gpio_ra_irq_data *data = dev->data;
	uint8_t irqcr = sys_read8(config->reg) & ~R_ICU_IRQCR_IRQMD_Msk;

	irq_disable(config->irq);

	if (callback->mode == GPIO_INT_MODE_LEVEL) {
		if (callback->trigger != GPIO_INT_TRIG_LOW) {
			return -ENOTSUP;
		}

		irqcr |= (EXT_INTERRUPT_EDGE_LOW_LEVEL & R_ICU_IRQCR_IRQMD_Msk);
	} else if (callback->mode == GPIO_INT_MODE_EDGE) {
		switch (callback->trigger) {
		case GPIO_INT_TRIG_LOW:
			irqcr |= (EXT_INTERRUPT_EDGE_FALLING & R_ICU_IRQCR_IRQMD_Msk);
			break;
		case GPIO_INT_TRIG_HIGH:
			irqcr |= (EXT_INTERRUPT_EDGE_RISING & R_ICU_IRQCR_IRQMD_Msk);
			break;
		case GPIO_INT_TRIG_BOTH:
			irqcr |= (EXT_INTERRUPT_EDGE_BOTH & R_ICU_IRQCR_IRQMD_Msk);
			break;
		default:
			return -ENOTSUP;
		}
	} else {
		return -ENOTSUP;
	}

	if (data->callback.port_num != callback->port_num || data->callback.pin != callback->pin) {
		if (0 != k_sem_take(&data->irq_sem, K_NO_WAIT)) {
			return -EBUSY;
		}
	}

	sys_write8(irqcr, config->reg);
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
void gpio_ra_interrupt_unset(const struct device *dev, uint8_t port_num, uint8_t pin)
{
	const struct gpio_ra_irq_config *config = dev->config;
	struct gpio_ra_irq_data *data = dev->data;

	if ((port_num != data->callback.port_num) && (pin != data->callback.pin)) {
		return;
	}

	irq_disable(config->irq);
	k_sem_give(&data->irq_sem);
}

void gpio_ra_isr(const struct device *dev)
{
	const struct gpio_ra_irq_data *data = dev->data;
	const struct gpio_ra_irq_config *config = dev->config;

	data->callback.isr(data->callback.port, data->callback.pin);
	R_BSP_IrqStatusClear(config->irq);
}

static int gpio_ra_interrupt_init(const struct device *dev)
{
	const struct gpio_ra_irq_config *config = dev->config;
	struct gpio_ra_irq_data *data = dev->data;
	uint8_t irqcr = ((config->trigger << R_ICU_IRQCR_IRQMD_Pos));

	WRITE_BIT(irqcr, R_ICU_IRQCR_FLTEN_Pos, config->digital_filter);
	sys_write8(irqcr, config->reg);
	k_sem_init(&data->irq_sem, 1, 1);

	return 0;
}

#define GPIO_INTERRUPT_INIT(index)                                                                 \
	static const struct gpio_ra_irq_config gpio_ra_irq_config##index = {                       \
		.reg = DT_INST_REG_ADDR(index),                                                    \
		.channel = DT_INST_PROP(index, channel),                                           \
		.trigger =                                                                         \
			DT_INST_ENUM_IDX_OR(index, renesas_trigger, EXT_INTERRUPT_EDGE_FALLING),   \
		.digital_filter = DT_INST_PROP_OR(index, renesas_digital_filtering, false),        \
		.sample_clock = UTIL_CAT(EXT_INTERRUPT_SAMPLE_CLOCK_DIV_,                          \
					 DT_INST_PROP_OR(index, renesas_sample_clock_div, 1)),     \
		.irq = DT_INST_IRQ(index, irq),                                                    \
	};                                                                                         \
	static struct gpio_ra_irq_data gpio_ra_irq_data##index;                                    \
	static int gpio_ra_irq_init##index(const struct device *dev)                               \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ(index, irq)] =                                            \
			UTIL_CAT(ELC_EVENT_ICU_IRQ, DT_INST_PROP(index, channel));                 \
		IRQ_CONNECT(DT_INST_IRQ(index, irq), DT_INST_IRQ(index, priority), gpio_ra_isr,    \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		return gpio_ra_interrupt_init(dev);                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, gpio_ra_irq_init##index, NULL, &gpio_ra_irq_data##index,      \
			      &gpio_ra_irq_config##index, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(GPIO_INTERRUPT_INIT)
