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

/*
 * Second benchmark IRQ line.
 *
 * Scenarios that compare two ways of connecting an interrupt need a
 * line of their own, because the connection method is fixed at build
 * time. The line number has to be visible here rather than staying
 * private to the backend, since IRQ_DIRECT_CONNECT() is expanded where
 * the handler is defined.
 *
 * RISC-V is absent on purpose: without a CLIC the machine software
 * interrupt is the only interrupt a hart can raise on itself, and the
 * first line already uses it.
 */
#if CONFIG_INT_BENCH_IRQ_LINE_ALT >= 0
#define BENCH_IRQ_LINE_ALT CONFIG_INT_BENCH_IRQ_LINE_ALT
#define BENCH_HAS_ALT_LINE 1
#elif defined(CONFIG_GIC)
/* SGI 0-2 are Zephyr's SMP IPIs and 8-15 may be Secure only */
#define BENCH_IRQ_LINE_ALT 7
#define BENCH_HAS_ALT_LINE 1
#elif defined(CONFIG_BOARD_QEMU_CORTEX_M3)
#define BENCH_IRQ_LINE_ALT 41
#define BENCH_HAS_ALT_LINE 1
#elif defined(CONFIG_X86)
#define BENCH_IRQ_LINE_ALT 28
#define BENCH_HAS_ALT_LINE 1
#elif defined(CONFIG_CPU_CORTEX_M) || defined(CONFIG_ARC)
#define BENCH_IRQ_LINE_ALT (CONFIG_NUM_IRQS - 2)
#define BENCH_HAS_ALT_LINE 1
#endif

#ifdef BENCH_HAS_ALT_LINE
/* Raise the interrupt on the second line */
void bench_trigger_alt(void);
#endif
#endif /* CONFIG_INT_BENCH_TRIGGER_SW_IRQ */

#endif /* ZEPHYR_TESTS_BENCHMARKS_INTERRUPT_LATENCY_SRC_TRIGGER_H_ */
