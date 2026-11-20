/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A user thread makes the SVC that arch_irq_offload() makes, with a kernel
 * function and its parameter in r0 and r1. The SVC handler must reject it
 * as a fatal error of the thread, without calling the function.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/arch/arm/syscall.h>

ZTEST_BMEM static volatile bool expect_fault;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);

	printk("Caught system error -- reason %u\n", reason);

	if (!expect_fault) {
		printk("Unexpected fault during test\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}

	expect_fault = false;
	printk("System error was expected\n");
}

static volatile uint32_t probe_called;

static void probe(const void *param)
{
	probe_called = POINTER_TO_UINT(param);
}

K_THREAD_STACK_DEFINE(user_stack, 1024);
static struct k_thread user_thread;

static void user_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	register uint32_t r0 __asm__("r0") = POINTER_TO_UINT(probe);
	register uint32_t r1 __asm__("r1") = 0x12345678U;

	__asm__ volatile("svc %[id]\n"
			 :
			 : [id] "i"(_SVC_CALL_IRQ_OFFLOAD), "r"(r0), "r"(r1)
			 : "memory");

	/* The SVC returned: let the join succeed and the checks fail */
}

ZTEST(arm_svc_user_offload, test_user_irq_offload_svc)
{
	probe_called = 0U;
	expect_fault = true;

	k_tid_t tid = k_thread_create(&user_thread, user_stack, K_THREAD_STACK_SIZEOF(user_stack),
				      user_entry, NULL, NULL, NULL, K_PRIO_PREEMPT(1), K_USER,
				      K_NO_WAIT);

	zassert_ok(k_thread_join(tid, K_SECONDS(10)), "user thread not aborted");
	zassert_false(expect_fault, "no fatal error");
	zassert_equal(probe_called, 0U, "the SVC called the kernel function");
}

ZTEST_SUITE(arm_svc_user_offload, NULL, NULL, NULL, NULL, NULL);
