/*
 * Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

ZTEST_BMEM static volatile bool expect_fault;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	printk("Caught system error -- reason %d\n", reason);

	if (!expect_fault) {
		printk("Unexpected fault during test\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	expect_fault = false;
	printk("System error was expected\n");
}

/* Kernel-only state that the EL0 SVC must never be able to touch. */
static volatile uint32_t probe_called;

/* Kernel function the user thread will try to invoke via svc #1. */
static void probe(uint32_t value)
{
	probe_called = value;
}

K_THREAD_STACK_DEFINE(evil_stack, 2048);
static struct k_thread evil_thread;

static void evil_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* arch_irq_offload() passes the routine in x0 and the argument in x1. */
	register uint64_t fn_reg __asm__("x0") = (uint64_t)(uintptr_t)probe;
	register uint64_t arg_reg __asm__("x1") = 0x12345678ULL;

	/* This is the raw SVC used by arch_irq_offload() from kernel mode. */
	__asm__ volatile("svc #1\n" : : "r"(fn_reg), "r"(arg_reg) : "memory");

	/* Reaching this point means the SVC was ignored instead of killing
	 * the thread. Return so the join below succeeds at once and the
	 * probe_called check fails immediately, rather than timing out.
	 */
}

ZTEST(svc_el0_gate, test_el0_cannot_trigger_irq_offload)
{
	probe_called = 0;
	expect_fault = true;

	k_tid_t tid =
		k_thread_create(&evil_thread, evil_stack, K_THREAD_STACK_SIZEOF(evil_stack),
				evil_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), K_USER, K_NO_WAIT);

	/* The thread must fault and be aborted by the fatal handler. */
	zassert_ok(k_thread_join(tid, K_SECONDS(10)), "EL0 svc #1 was not rejected");

	zassert_equal(probe_called, 0, "svc #1 from EL0 invoked kernel code at EL1");
}

ZTEST_SUITE(svc_el0_gate, NULL, NULL, NULL, NULL, NULL);
