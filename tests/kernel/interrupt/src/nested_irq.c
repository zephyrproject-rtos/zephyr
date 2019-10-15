/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "interrupt.h"

#define DURATION 5
struct k_timer timer;

/* This tests uses two IRQ lines, selected within the range of IRQ lines
 * available on the target SOC the test executes on (and starting from
 * the maximum available IRQ line index)
 */
#define IRQ_LINE(offset) (CONFIG_NUM_IRQS - ((offset) + 1))

#define ISR0_OFFSET 1
#define ISR1_OFFSET 2

/* Keeping isr0 to be lowest priority than system timer
 * so that it can be interrupted by timer triggered.
 * In NRF5, RTC system timer is of priority 1 and
 * in all other architectures, system timer is considered
 * to be in priority 0.
 */
#if defined(CONFIG_CPU_CORTEX_M)
u32_t irq_line_0;
u32_t irq_line_1;
#define ISR0_PRIO 2
#define ISR1_PRIO 1
#else
#define ISR0_PRIO 1
#define ISR1_PRIO 0
#endif /* CONFIG_CPU_CORTEX_M */

#define MS_TO_US(ms)  (K_MSEC(ms) * USEC_PER_MSEC)
volatile u32_t new_val;
u32_t old_val = 0xDEAD;
volatile u32_t check_lock_new;
u32_t check_lock_old = 0xBEEF;

void isr1(void *param)
{
	ARG_UNUSED(param);
	new_val = 0xDEAD;
	printk("%s ran !!\n", __func__);
}

/**
 *
 * triggering interrupt from the timer expiry function while isr0
 * is in busy wait
 */
#ifndef NO_TRIGGER_FROM_SW
static void handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
#if defined(CONFIG_CPU_CORTEX_M)
	irq_enable(irq_line_1);
	trigger_irq(irq_line_1);
#else
	irq_enable(IRQ_LINE(ISR1_OFFSET));
	trigger_irq(IRQ_LINE(ISR1_OFFSET));
#endif /* CONFIG_CPU_CORTEX_M */
}
#else
void handler(void)
{
	ztest_test_skip();
}
#endif /* NO_TRIGGER_FROM_SW */

void isr0(void *param)
{
	ARG_UNUSED(param);
	printk("%s running !!\n", __func__);
#if defined(CONFIG_BOARD_QEMU_CORTEX_M0)
	/* QEMU Cortex-M0 timer emulation appears to not capturing the
	 * current time accurately, resulting in erroneous busy wait
	 * implementation.
	 *
	 * Work-around:
	 * Increase busy-loop duration to ensure the timer interrupt will fire
	 * during the busy loop waiting.
	 */
	k_busy_wait(MS_TO_US(1000));
#else
	k_busy_wait(MS_TO_US(10));
#endif
	printk("%s execution completed !!\n", __func__);
	zassert_equal(new_val, old_val, "Nested interrupt is not working\n");
}

/**
 *
 * Interrupt nesting feature allows an ISR to be preempted in mid-execution
 * if a higher priority interrupt is signaled. The lower priority ISR resumes
 * execution once the higher priority ISR has completed its processing.
 * The expected control flow should be isr0 -> handler -> isr1 -> isr0
 */
#ifndef NO_TRIGGER_FROM_SW
void test_nested_isr(void)
{
#if defined(CONFIG_CPU_CORTEX_M)
	irq_line_0 = get_available_nvic_line(CONFIG_NUM_IRQS);
	irq_line_1 = get_available_nvic_line(irq_line_0);
	z_arch_irq_connect_dynamic(irq_line_0, ISR0_PRIO, isr0, NULL, 0);
	z_arch_irq_connect_dynamic(irq_line_1, ISR1_PRIO, isr1, NULL, 0);
#else
	IRQ_CONNECT(IRQ_LINE(ISR0_OFFSET), ISR0_PRIO, isr0, NULL, 0);
	IRQ_CONNECT(IRQ_LINE(ISR1_OFFSET), ISR1_PRIO, isr1, NULL, 0);
#endif /* CONFIG_CPU_CORTEX_M */

	k_timer_init(&timer, handler, NULL);
	k_timer_start(&timer, DURATION, K_NO_WAIT);

#if defined(CONFIG_CPU_CORTEX_M)
	irq_enable(irq_line_0);
	trigger_irq(irq_line_0);
#else
	irq_enable(IRQ_LINE(ISR0_OFFSET));
	trigger_irq(IRQ_LINE(ISR0_OFFSET));
#endif /* CONFIG_CPU_CORTEX_M */

}
#else
void test_nested_isr(void)
{
	ztest_test_skip();
}
#endif /* NO_TRIGGER_FROM_SW */

static void timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	check_lock_new = 0xBEEF;
}

static void offload_function(void *param)
{
	ARG_UNUSED(param);

	zassert_true(k_is_in_isr(), "Not in IRQ context!");
	k_timer_init(&timer, timer_handler, NULL);
	k_busy_wait(MS_TO_US(1));
	k_timer_start(&timer, DURATION, K_NO_WAIT);
	zassert_not_equal(check_lock_new, check_lock_old,
		"Interrupt locking didn't work properly");
}

void test_prevent_interruption(void)
{
	irq_offload(offload_function, NULL);
	k_timer_stop(&timer);
}
