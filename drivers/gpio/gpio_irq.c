/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio/gpio_irq.h>

static void gpio_irq_callback_handler(const struct device *port,
				      struct gpio_callback *cb,
				      gpio_port_pins_t pins)
{
	ARG_UNUSED(pins);

	struct gpio_irq *irq =
		CONTAINER_OF(cb, struct gpio_irq, nested_callback);

	irq->handler(irq);
}

int gpio_irq_request(const struct device *controller,
		     gpio_pin_t irq_pin,
		     gpio_dt_flags_t irq_flags,
		     struct gpio_irq *irq,
		     gpio_irq_callback_handler_t handler)
{
	gpio_flags_t gpio_flags;

	irq->controller = controller;
	irq->irq_pin = irq_pin;
	irq->irq_flags = irq_flags;
	irq->handler = handler;

	if (!device_is_ready(controller)) {
		return -ENODEV;
	}

	if (!IRQ_TYPE_IS_VALID(irq_flags)) {
		return -EINVAL;
	}

	gpio_flags = irq_flags & (~IRQ_TYPE_MASK);
	if (gpio_pin_configure(controller, irq_pin, GPIO_INPUT) < 0) {
		return -EINVAL;
	}

	gpio_init_callback(&irq->nested_callback, gpio_irq_callback_handler, BIT(irq_pin));
	if (gpio_add_callback(controller, &irq->nested_callback) < 0) {
		return -EINVAL;
	}

	if (gpio_irq_enable(irq) < 0) {
		(void)gpio_remove_callback(controller, &irq->nested_callback);
		return -EAGAIN;
	}

	return 0;
}

bool gpio_irq_dt_spec_exists(const struct gpio_irq_dt_spec *spec)
{
	return (spec->controller != NULL);
}

int gpio_irq_request_dt(const struct gpio_irq_dt_spec *spec,
			struct gpio_irq *irq,
			gpio_irq_callback_handler_t handler)
{
	return gpio_irq_request(spec->controller, spec->irq_pin, spec->irq_flags, irq, handler);
}

int gpio_irq_release(struct gpio_irq *irq)
{
	(void)gpio_irq_disable(irq);
	return gpio_remove_callback(irq->controller, &irq->nested_callback);
}

int gpio_irq_enable(struct gpio_irq *irq)
{
	gpio_flags_t gpio_flags = GPIO_INT_ENABLE;

	if (IRQ_TYPE_IS_EDGE_TRIGGERED(irq->irq_flags)) {
		gpio_flags |= GPIO_INT_EDGE;
	}

	if (IRQ_TYPE_IS_ACTIVE_HIGH(irq->irq_flags)) {
		gpio_flags |= GPIO_INT_HIGH_1;
	}

	if (IRQ_TYPE_IS_ACTIVE_LOW(irq->irq_flags)) {
		gpio_flags |= GPIO_INT_LOW_0;
	}

	return gpio_pin_interrupt_configure(irq->controller, irq->irq_pin, gpio_flags);
}

int gpio_irq_disable(struct gpio_irq *irq)
{
	return gpio_pin_interrupt_configure(irq->controller, irq->irq_pin, GPIO_INT_DISABLE);
}
