/*
 * Copyright (c) 2025 Pedro Ramos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/energy_meter.h>
#include <zephyr/logging/log.h>

#include "atm90e32as.h"

LOG_MODULE_DECLARE(atm90e32as, CONFIG_ENERGY_METER_LOG_LEVEL);

static const struct energy_meter_event atm90e32as_line_events[ATM90E32AS_IRQ_LINES] = {
	[ATM90E32AS_IRQ0] = {.type = ENERGY_METER_EVENT_ALARM, .index = 0},
	[ATM90E32AS_IRQ1] = {.type = ENERGY_METER_EVENT_ALARM, .index = 1},
	[ATM90E32AS_WARN] = {.type = ENERGY_METER_EVENT_FAULT, .index = 0},
	[ATM90E32AS_CF1] = {.type = ENERGY_METER_EVENT_PULSE, .index = 0},
	[ATM90E32AS_CF2] = {.type = ENERGY_METER_EVENT_PULSE, .index = 1},
	[ATM90E32AS_CF3] = {.type = ENERGY_METER_EVENT_PULSE, .index = 2},
	[ATM90E32AS_CF4] = {.type = ENERGY_METER_EVENT_PULSE, .index = 3},
};

static void atm90e32as_event_work(struct k_work *work)
{
	struct atm90e32as_irq_ctx *ctx = CONTAINER_OF(work, struct atm90e32as_irq_ctx, work);
	struct atm90e32as_data *data = ctx->dev->data;
	energy_meter_event_cb_t cb = data->event_cb;

	if (cb != NULL) {
		cb(ctx->dev, &ctx->event, data->event_user_data);
	}
}

static void atm90e32as_gpio_callback(const struct device *port, struct gpio_callback *cb,
				     uint32_t pins)
{
	struct atm90e32as_irq_ctx *ctx = CONTAINER_OF(cb, struct atm90e32as_irq_ctx, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&ctx->work);
}

void atm90e32as_events_enable(const struct device *dev, bool enable)
{
	const struct atm90e32as_config *cfg = dev->config;
	struct atm90e32as_data *data = dev->data;
	gpio_flags_t flags =
		(enable && (data->event_cb != NULL)) ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	for (int line = 0; line < ATM90E32AS_IRQ_LINES; line++) {
		if (cfg->irq_gpios[line].port == NULL) {
			continue;
		}

		if (gpio_pin_interrupt_configure_dt(&cfg->irq_gpios[line], flags) < 0) {
			LOG_WRN("Could not configure interrupt %d", line);
		}
	}
}

int atm90e32as_set_event_callback(const struct device *dev, energy_meter_event_cb_t cb,
				  void *user_data)
{
	struct atm90e32as_data *data = dev->data;

	atm90e32as_events_enable(dev, false);

	data->event_cb = cb;
	data->event_user_data = user_data;

	atm90e32as_events_enable(dev, true);

	return 0;
}

int atm90e32as_events_init(const struct device *dev)
{
	const struct atm90e32as_config *cfg = dev->config;
	struct atm90e32as_data *data = dev->data;
	int ret;

	for (int line = 0; line < ATM90E32AS_IRQ_LINES; line++) {
		const struct gpio_dt_spec *gpio = &cfg->irq_gpios[line];
		struct atm90e32as_irq_ctx *ctx = &data->irq[line];

		if (gpio->port == NULL) {
			continue;
		}

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR("Interrupt GPIO %d not ready", line);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
		if (ret < 0) {
			return ret;
		}

		ctx->dev = dev;
		ctx->event = atm90e32as_line_events[line];
		k_work_init(&ctx->work, atm90e32as_event_work);
		gpio_init_callback(&ctx->gpio_cb, atm90e32as_gpio_callback, BIT(gpio->pin));

		ret = gpio_add_callback_dt(gpio, &ctx->gpio_cb);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}
