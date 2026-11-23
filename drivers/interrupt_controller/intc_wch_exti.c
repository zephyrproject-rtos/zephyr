/*
 * Copyright (c) 2025 Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_exti

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/interrupt_controller/wch_exti.h>

#include <hal_ch32fun.h>

#define WCH_EXTI_NUM_LINES DT_PROP(DT_NODELABEL(exti), num_lines)

/* Per EXTI callback registration */
struct wch_exti_registration {
	wch_exti_callback_handler_t callback;
	void *user;
};

struct wch_exti_data {
	struct wch_exti_registration callbacks[WCH_EXTI_NUM_LINES];
};

#define WCH_EXTI_INIT_RANGE(node_id, interrupts, idx)                                              \
	DT_PROP_BY_IDX(node_id, line_ranges, UTIL_X2(idx)),

/*
 * List of [start, end) line ranges for each line group, where the range for group n is
 * `[wch_exti_ranges[n-1]...wch_exti_ranges[n])`. This uses the fact that the ranges are contiguous,
 * so the end of group n is the same as the start of group n+1.
 */
static const uint8_t wch_exti_ranges[] = {
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(exti), interrupt_names, WCH_EXTI_INIT_RANGE)
		WCH_EXTI_NUM_LINES,
};

#define WCH_EXTI_INIT_INTERRUPT(node_id, interrupts, idx) DT_IRQ_BY_IDX(node_id, idx, irq),

/* Interrupt number for each line group. Used when enabling the interrupt. */
static const uint8_t wch_exti_interrupts[] = {
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(exti), interrupt_names, WCH_EXTI_INIT_INTERRUPT)};

BUILD_ASSERT(ARRAY_SIZE(wch_exti_interrupts) + 1 == ARRAY_SIZE(wch_exti_ranges));

static void wch_exti_isr(const void *user)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct wch_exti_data *data = dev->data;
	EXTI_TypeDef *regs = (EXTI_TypeDef *)DT_INST_REG_ADDR(0);
	const uint8_t *range = user;
	uint32_t intfr = regs->INTFR;

	for (uint8_t line = range[0]; line < range[1]; line++) {
		if ((intfr & BIT(line)) != 0) {
			const struct wch_exti_registration *callback = &data->callbacks[line];
			/* Clear the interrupt */
			regs->INTFR = BIT(line);
			if (callback->callback != NULL) {
				callback->callback(line, callback->user);
			}
		}
	}
}

void wch_exti_enable(uint8_t line)
{
	EXTI_TypeDef *regs = (EXTI_TypeDef *)DT_INST_REG_ADDR(0);

	regs->INTENR |= BIT(line);
	/* Find the corresponding interrupt and enable it */
	for (uint8_t i = 1; i < ARRAY_SIZE(wch_exti_ranges); i++) {
		if (line < wch_exti_ranges[i]) {
			irq_enable(wch_exti_interrupts[i - 1]);
			break;
		}
	}
}

void wch_exti_disable(uint8_t line)
{
	EXTI_TypeDef *regs = (EXTI_TypeDef *)DT_INST_REG_ADDR(0);

	regs->INTENR &= ~BIT(line);
}

int wch_exti_configure(uint8_t line, wch_exti_callback_handler_t callback, void *user)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct wch_exti_data *data = dev->data;
	struct wch_exti_registration *registration = &data->callbacks[line];

	if (registration->callback == callback && registration->user == user) {
		return 0;
	}

	if (callback != NULL && registration->callback != NULL) {
		return -EALREADY;
	}

	registration->callback = callback;
	registration->user = user;

	return 0;
}

void wch_exti_set_trigger(uint8_t line, enum wch_exti_trigger trigger)
{
	EXTI_TypeDef *regs = (EXTI_TypeDef *)DT_INST_REG_ADDR(0);

	WRITE_BIT(regs->RTENR, line, (trigger & WCH_EXTI_TRIGGER_RISING_EDGE) != 0);
	WRITE_BIT(regs->FTENR, line, (trigger & WCH_EXTI_TRIGGER_FALLING_EDGE) != 0);
}

#define WCH_EXTI_CONNECT_IRQ(node_id, interrupts, idx)                                             \
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq), DT_IRQ_BY_IDX(node_id, idx, priority),       \
		    wch_exti_isr, &wch_exti_ranges[idx], 0);

static int wch_exti_init(const struct device *dev)
{
	/* Generate the registrations for each interrupt */
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(exti), interrupt_names, WCH_EXTI_CONNECT_IRQ);

	return 0;
}

static struct wch_exti_data wch_exti_data_0;

DEVICE_DT_INST_DEFINE(0, wch_exti_init, NULL, &wch_exti_data_0, NULL, PRE_KERNEL_2,
		      CONFIG_INTC_INIT_PRIORITY, NULL);
