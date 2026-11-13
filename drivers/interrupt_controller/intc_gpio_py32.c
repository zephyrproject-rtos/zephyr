/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puya_py32_exti

#include <soc.h>
#include <py32f0xx_ll_exti.h>
#include <zephyr/drivers/interrupt_controller/gpio_intc_py32.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

struct py32_exti_line {
	py32_exti_callback_t cb;
	void *user;
	uint32_t port;
};

static struct py32_exti_line lines[16];

static uint32_t py32_exti_src_line(gpio_pin_t line)
{
	uint32_t mask = (line < 5U) ? 0x3U : 0x1U;

	return (mask << LL_EXTI_REGISTER_PINMASK_SHFT) |
	       ((line % 4U * 8U) << LL_EXTI_REGISTER_PINPOS_SHFT) | (line / 4U);
}

void py32_exti_set_line_src_port(gpio_pin_t line, uint32_t port)
{
	/* PA9-PA15 have no source selector. GPIOF uses source encoding 2. */
	if (line < 9U) {
		LL_EXTI_SetEXTISource(port == 5U ? LL_EXTI_CONFIG_PORTF : port,
				      py32_exti_src_line(line));
	}
}

uint32_t py32_exti_get_line_src_port(gpio_pin_t line)
{
	uint32_t port;

	if (line >= 9U) {
		return 0U;
	}
	port = LL_EXTI_GetEXTISource(py32_exti_src_line(line));
	return port == LL_EXTI_CONFIG_PORTF ? 5U : port;
}

int py32_exti_configure(uint32_t port, gpio_pin_t pin, enum gpio_int_mode mode,
			enum gpio_int_trig trig, py32_exti_callback_t cb, void *user)
{
	uint32_t mask;
	unsigned int key;

	if ((pin >= ARRAY_SIZE(lines)) || ((port != 0U) && (port != 1U) && (port != 5U))) {
		return -EINVAL;
	}
	if (((pin >= 9U) && (port != 0U)) || ((pin >= 5U) && (port == 5U))) {
		return mode == GPIO_INT_MODE_DISABLED ? 0 : -ENOTSUP;
	}
	if ((mode != GPIO_INT_MODE_DISABLED) && (mode != GPIO_INT_MODE_EDGE)) {
		return -ENOTSUP;
	}
	mask = BIT(pin);
	key = irq_lock();
	if ((lines[pin].cb != NULL) && (lines[pin].port != port)) {
		irq_unlock(key);
		return mode == GPIO_INT_MODE_DISABLED ? 0 : -EBUSY;
	}
	LL_EXTI_DisableIT(mask);
	LL_EXTI_DisableRisingTrig(mask);
	LL_EXTI_DisableFallingTrig(mask);
	LL_EXTI_ClearFlag(mask);
	lines[pin].cb = NULL;
	if (mode != GPIO_INT_MODE_DISABLED) {
		py32_exti_set_line_src_port(pin, port);
		lines[pin].port = port;
		lines[pin].user = user;
		lines[pin].cb = cb;
		if ((trig & GPIO_INT_TRIG_HIGH) != 0U) {
			LL_EXTI_EnableRisingTrig(mask);
		}
		if ((trig & GPIO_INT_TRIG_LOW) != 0U) {
			LL_EXTI_EnableFallingTrig(mask);
		}
		LL_EXTI_EnableIT(mask);
	}
	irq_unlock(key);
	return 0;
}

static void py32_exti_isr(const void *arg)
{
	uint32_t range = POINTER_TO_UINT(arg);
	uint32_t pending = EXTI->PR & EXTI->IMR & range;

	LL_EXTI_ClearFlag(pending);
	for (gpio_pin_t pin = 0U; pin < ARRAY_SIZE(lines); pin++) {
		if (((pending & BIT(pin)) != 0U) && (lines[pin].cb != NULL)) {
			lines[pin].cb(BIT(pin), lines[pin].user);
		}
	}
}

#define PY32_EXTI_IRQ(node_id, prop, idx)                                                          \
	IRQ_CONNECT(                                                                               \
		DT_IRQ_BY_IDX(node_id, idx, irq), DT_IRQ_BY_IDX(node_id, idx, priority),           \
		py32_exti_isr,                                                                     \
		UINT_TO_POINTER(GENMASK(                                                           \
			DT_PROP_BY_IDX(node_id, line_ranges, UTIL_X2(idx)) +                       \
				DT_PROP_BY_IDX(node_id, line_ranges, UTIL_INC(UTIL_X2(idx))) - 1U, \
			DT_PROP_BY_IDX(node_id, line_ranges, UTIL_X2(idx)))),                      \
		0);                                                                                \
	irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq));

static int py32_exti_init(void)
{
	DT_FOREACH_PROP_ELEM(DT_DRV_INST(0), interrupt_names, PY32_EXTI_IRQ)
	return 0;
}

SYS_INIT(py32_exti_init, PRE_KERNEL_1, CONFIG_INTC_INIT_PRIORITY);
