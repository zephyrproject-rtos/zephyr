/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BENCHMARKS_INTERRUPT_LATENCY_SRC_TRIGGER_H_
#define ZEPHYR_TESTS_BENCHMARKS_INTERRUPT_LATENCY_SRC_TRIGGER_H_

#include <zephyr/kernel.h>

/*
 * Trigger backend abstraction.
 *
 * A backend provides a way to run a benchmark-controlled handler in
 * interrupt context. The sw-irq backend raises a real asynchronous
 * interrupt through the platform interrupt controller; the offload
 * backend falls back to the synchronous irq_offload() trap. Scenarios
 * install their measurement handler with bench_trigger_set_handler()
 * and fire it with bench_trigger().
 */
typedef void (*bench_trigger_handler_t)(void);

int bench_trigger_init(void);
void bench_trigger_set_handler(bench_trigger_handler_t handler);
void bench_trigger(void);

/* ISR wrapper installed on the benchmark IRQ line */
void bench_trigger_isr(const void *arg);

#ifdef CONFIG_INT_BENCH_TRIGGER_SW_IRQ
unsigned int bench_trigger_irq_line(void);
#endif

#endif /* ZEPHYR_TESTS_BENCHMARKS_INTERRUPT_LATENCY_SRC_TRIGGER_H_ */
