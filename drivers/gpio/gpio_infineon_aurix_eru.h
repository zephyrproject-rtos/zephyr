/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* GPIO pin interrupts through the external request unit */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_INFINEON_AURIX_ERU_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_INFINEON_AURIX_ERU_H_

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#include <aurix_eru.h>

/* cell: pin, ERU input channel, request input, ERU output channel */
struct gpio_aurix_eru_input {
	const struct device *dev;
	const uint8_t *cell;
};

#define ERU_PIN(in)     ((in)->cell[0])
#define ERU_CHANNEL(in) ((in)->cell[1])
#define ERU_INPUT(in)   ((in)->cell[2])
#define ERU_OUTPUT(in)  ((in)->cell[3])

static inline int gpio_aurix_eru_check(const struct gpio_aurix_eru_input *inputs, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		if (ERU_CHANNEL(&inputs[i]) >= AURIX_ERU_INPUTS ||
		    ERU_OUTPUT(&inputs[i]) >= AURIX_ERU_OUTPUTS) {
			return -EINVAL;
		}
	}

	return 0;
}

static inline const struct gpio_aurix_eru_input *
gpio_aurix_eru_find(const struct gpio_aurix_eru_input *inputs, size_t count, gpio_pin_t pin)
{
	for (size_t i = 0; i < count; i++) {
		if (ERU_PIN(&inputs[i]) == pin) {
			return &inputs[i];
		}
	}

	return NULL;
}

static inline int gpio_aurix_eru_configure(const struct gpio_aurix_eru_input *inputs, size_t count,
					   gpio_pin_t pin, enum gpio_int_mode mode,
					   enum gpio_int_trig trig)
{
	const struct gpio_aurix_eru_input *in = gpio_aurix_eru_find(inputs, count, pin);

	if (in == NULL) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		aurix_eru_configure(ERU_CHANNEL(in), ERU_INPUT(in), ERU_OUTPUT(in), false, false);
		return 0;
	}

	/* The ERU detects edges only */
	if (mode != GPIO_INT_MODE_EDGE) {
		return -ENOTSUP;
	}

	aurix_eru_configure(ERU_CHANNEL(in), ERU_INPUT(in), ERU_OUTPUT(in),
			    (trig & GPIO_INT_TRIG_HIGH) != 0U, (trig & GPIO_INT_TRIG_LOW) != 0U);

	return 0;
}

static inline void gpio_aurix_eru_isr(const struct gpio_aurix_eru_input *in, sys_slist_t *callbacks)
{
	aurix_eru_clear(ERU_CHANNEL(in));
	gpio_fire_callbacks(callbacks, in->dev, BIT(ERU_PIN(in)));
}

/* One interrupt per entry; DT_INST_NUM_IRQS() is a literal, as LISTIFY() needs */
#define GPIO_AURIX_ERU_COUNT(n) DT_INST_NUM_IRQS(n)

#define GPIO_AURIX_ERU_INPUT(i, n)                                                                 \
	{.dev = DEVICE_DT_INST_GET(n), .cell = &gpio_aurix_eru_cells_##n[(i) * 4]}

#define GPIO_AURIX_ERU_IRQ(i, n, isr)                                                              \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, i, irq), DT_INST_IRQ_BY_IDX(n, i, priority), isr,        \
		    &gpio_aurix_eru_##n[i], 0);                                                    \
	irq_enable(DT_INST_IRQ_BY_IDX(n, i, irq));

/* Table of the ERU inputs of instance n and the function connecting their interrupts */
#define GPIO_AURIX_ERU_DEFINE(n, isr)                                                              \
	BUILD_ASSERT(DT_INST_PROP_LEN_OR(n, infineon_eru_inputs, 0) == 4 * DT_INST_NUM_IRQS(n),    \
		     "infineon,eru-inputs needs four cells per interrupt");                        \
	static const uint8_t gpio_aurix_eru_cells_##n[] __maybe_unused =                           \
		DT_INST_PROP_OR(n, infineon_eru_inputs, {});                                       \
	static const struct gpio_aurix_eru_input gpio_aurix_eru_##n[] = {                          \
		LISTIFY(GPIO_AURIX_ERU_COUNT(n), GPIO_AURIX_ERU_INPUT, (,), n)};                   \
	static void gpio_aurix_eru_irq_config_##n(void)                                            \
	{                                                                                          \
		LISTIFY(GPIO_AURIX_ERU_COUNT(n), GPIO_AURIX_ERU_IRQ, (), n, isr)                   \
	}

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_INFINEON_AURIX_ERU_H_ */
