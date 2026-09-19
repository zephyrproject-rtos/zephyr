/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Fallback trigger backend based on irq_offload(). Portable to every
 * architecture, but the handler runs from a synchronous software trap
 * rather than an asynchronous interrupt: only the interrupt exit paths
 * are meaningfully measured with this backend.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>

#include "trigger.h"

static bench_trigger_handler_t trigger_handler;

BENCH_ISR_FUNC void bench_trigger_isr(const void *arg)
{
	ARG_UNUSED(arg);

	if (trigger_handler != NULL) {
		trigger_handler();
	}
}

void bench_trigger_set_handler(bench_trigger_handler_t handler)
{
	trigger_handler = handler;
}

int bench_trigger_init(void)
{
	return 0;
}

void bench_trigger(void)
{
	irq_offload(bench_trigger_isr, NULL);
}
