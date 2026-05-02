/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>

#include <zephyr/kernel/thread_stack.h>

#define IV_CTRL_PROTECTION_EXCEPTION 21

#define CTRL_PROTECTION_ERRORCODE_NEAR_RET 1
#define CTRL_PROTECTION_ERRORCODE_ENDBRANCH 3

#define STACKSIZE 1024
#define THREAD_PRIORITY 5

K_SEM_DEFINE(error_handler_sem, 0, 1);

volatile bool expect_fault;
volatile int expect_code;
volatile int expect_reason;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	if (expect_fault) {
#ifdef CONFIG_X86_64
		zassert_equal(pEsf->vector, expect_reason, "unexpected exception");
		zassert_equal(pEsf->code, expect_code, "unexpected error code");
#else
		zassert_equal(z_x86_exception_vector, expect_reason, "unexpected exception");
		zassert_equal(pEsf->errorCode, expect_code, "unexpected error code");
#endif
		printk("fatal error expected as part of test case\n");
		expect_fault = false;

		k_sem_give(&error_handler_sem);
	} else {
		printk("fatal error was unexpected, aborting\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

#ifdef CONFIG_HW_SHADOW_STACK
void thread_a_entry(void *p1, void *p2, void *p3);
K_SEM_DEFINE(thread_a_sem, 0, 1);
K_THREAD_DEFINE(thread_a, STACKSIZE, thread_a_entry, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

void thread_b_entry(void *p1, void *p2, void *p3);
K_SEM_DEFINE(thread_b_sem, 0, 1);
K_SEM_DEFINE(thread_b_irq_sem, 0, 1);
K_THREAD_DEFINE(thread_b, STACKSIZE, thread_b_entry, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

static bool is_shstk_enabled(void)
{
	long cur;

	cur = z_x86_msr_read(X86_S_CET_MSR);
	return (cur & X86_S_CET_MSR_SHSTK_EN) == X86_S_CET_MSR_SHSTK_EN;
}

void thread_c_entry(void *p1, void *p2, void *p3)
{
	zassert_true(is_shstk_enabled(), "shadow stack not enabled on static thread");
}

K_THREAD_DEFINE(thread_c, STACKSIZE, thread_c_entry, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, 0);

void __attribute__((optimize("O0"))) foo(void)
{
	printk("foo called\n");
}

void __attribute__((optimize("O0"))) fail(void)
{
	long a[] = {0};

	printk("should fail after this\n");

	*(a + 2) = (long)&foo;
}

struct k_work work;

void work_handler(struct k_work *wrk)
{
	printk("work handler\n");

	zassert_true(is_shstk_enabled(), "shadow stack not enabled");
}

ZTEST(cet, test_shstk_work_q)
{
	k_work_init(&work, work_handler);
	k_work_submit(&work);
}

void intr_handler(const void *p)
{
	printk("interrupt handler\n");

	if (p != NULL) {
		/* Test one nested level. It should just work. */
		printk("trying interrupt handler\n");
		irq_offload(intr_handler, NULL);

		k_sem_give((struct k_sem *)p);
	} else {
		printk("interrupt handler nested\n");
	}
}

void thread_b_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&thread_b_sem, K_FOREVER);

	irq_offload(intr_handler, &thread_b_irq_sem);

	k_sem_take(&thread_b_irq_sem, K_FOREVER);
}

ZTEST(cet, test_shstk_irq)
{
	k_thread_start(thread_b);

	k_sem_give(&thread_b_sem);

	k_thread_join(thread_b, K_FOREVER);
}

void thread_a_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&thread_a_sem, K_FOREVER);

	fail();

	zassert_unreachable("should not reach here");
}

ZTEST(cet, test_shstk)
{
	k_thread_start(thread_a);

	expect_fault = true;
	expect_code = CTRL_PROTECTION_ERRORCODE_NEAR_RET;
	expect_reason = IV_CTRL_PROTECTION_EXCEPTION;
	k_sem_give(&thread_a_sem);

	k_sem_take(&error_handler_sem, K_FOREVER);
	k_thread_abort(thread_a);
}
#endif /* CONFIG_HW_SHADOW_STACK */

#ifdef CONFIG_X86_CET_IBT
extern int should_work(int a);
extern int should_not_work(int a);

/* Round trip to trick optimisations and ensure the calls are indirect */
int do_call(int (*func)(int), int a)
{
	return func(a);
}

ZTEST(cet, test_ibt)
{
	zassert_equal(do_call(should_work, 1), 2, "should_work failed");

	expect_fault = true;
	expect_code = CTRL_PROTECTION_ERRORCODE_ENDBRANCH;
	expect_reason = IV_CTRL_PROTECTION_EXCEPTION;
	do_call(should_not_work, 1);
	zassert_unreachable("should_not_work did not fault");
}
#endif /* CONFIG_X86_CET_IBT */

ZTEST_SUITE(cet, NULL, NULL, NULL, NULL, NULL);
