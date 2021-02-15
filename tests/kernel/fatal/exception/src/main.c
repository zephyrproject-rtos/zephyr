/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>
#include <kernel_structs.h>
#include <irq_offload.h>
#include <kswap.h>
#include <assert.h>

#if defined(CONFIG_USERSPACE)
#include <syscall_handler.h>
#include "test_syscalls.h"
#endif

#if defined(CONFIG_X86) && defined(CONFIG_X86_MMU)
#define STACKSIZE (8192)
#else
#define  STACKSIZE (2048 + CONFIG_TEST_EXTRA_STACKSIZE)
#endif
#define MAIN_PRIORITY 7
#define PRIORITY 5

static K_THREAD_STACK_DEFINE(alt_stack, STACKSIZE);

#if defined(CONFIG_STACK_SENTINEL) && !defined(CONFIG_ARCH_POSIX)
#define OVERFLOW_STACKSIZE (STACKSIZE / 2)
static k_thread_stack_t *overflow_stack =
		alt_stack + (STACKSIZE - OVERFLOW_STACKSIZE);
#else
#if defined(CONFIG_USERSPACE) && defined(CONFIG_ARC)
/* for ARC, privilege stack is merged into defined stack */
#define OVERFLOW_STACKSIZE (STACKSIZE + CONFIG_PRIVILEGED_STACK_SIZE)
#else
#define OVERFLOW_STACKSIZE STACKSIZE
#endif
#endif

static struct k_thread alt_thread;
volatile int rv;

static ZTEST_DMEM volatile int expected_reason = -1;

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	TC_PRINT("Caught system error -- reason %d\n", reason);

	if (expected_reason == -1) {
		printk("Was not expecting a crash\n");
		k_fatal_halt(reason);
	}

	if (k_current_get() != &alt_thread) {
		printk("Wrong thread crashed\n");
		k_fatal_halt(reason);
	}

	if (reason != expected_reason) {
		printk("Wrong crash type got %d expected %d\n", reason,
		       expected_reason);
		k_fatal_halt(reason);
	}

	expected_reason = -1;
}

void entry_cpu_exception(void *p1, void *p2, void *p3)
{
	expected_reason = K_ERR_CPU_EXCEPTION;

#if defined(CONFIG_X86)
	__asm__ volatile ("ud2");
#elif defined(CONFIG_NIOS2)
	__asm__ volatile ("trap");
#elif defined(CONFIG_ARC)
	__asm__ volatile ("swi");
#else
	/* Triggers usage fault on ARM, illegal instruction on RISCV
	 * and xtensa
	 */
	{
		volatile long illegal = 0;
		((void(*)(void))&illegal)();
	}
#endif
	rv = TC_FAIL;
}


void entry_oops(void *p1, void *p2, void *p3)
{
	unsigned int key;

	expected_reason = K_ERR_KERNEL_OOPS;

	key = irq_lock();
	k_oops();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

void entry_panic(void *p1, void *p2, void *p3)
{
	unsigned int key;

	expected_reason = K_ERR_KERNEL_PANIC;

	key = irq_lock();
	k_panic();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

void entry_zephyr_assert(void *p1, void *p2, void *p3)
{
	expected_reason = K_ERR_KERNEL_PANIC;

	__ASSERT(0, "intentionally failed assertion");
	rv = TC_FAIL;
}

void entry_arbitrary_reason(void *p1, void *p2, void *p3)
{
	unsigned int key;

	expected_reason = INT_MAX;

	key = irq_lock();
	z_except_reason(INT_MAX);
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

#ifndef CONFIG_ARCH_POSIX
#ifdef CONFIG_STACK_SENTINEL
__no_optimization void blow_up_stack(void)
{
	char buf[OVERFLOW_STACKSIZE];

	expected_reason = K_ERR_STACK_CHK_FAIL;
	TC_PRINT("posting %zu bytes of junk to stack...\n", sizeof(buf));
	(void)memset(buf, 0xbb, sizeof(buf));
}
#else
/* stack sentinel doesn't catch it in time before it trashes the entire kernel
 */
__no_optimization int stack_smasher(int val)
{
	return stack_smasher(val * 2) + stack_smasher(val * 3);
}

void blow_up_stack(void)
{
	expected_reason = K_ERR_STACK_CHK_FAIL;

	stack_smasher(37);
}

#if defined(CONFIG_USERSPACE)

void z_impl_blow_up_priv_stack(void)
{
	blow_up_stack();
}

static inline void z_vrfy_blow_up_priv_stack(void)
{
	z_impl_blow_up_priv_stack();
}
#include <syscalls/blow_up_priv_stack_mrsh.c>

#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_STACK_SENTINEL */

void stack_sentinel_timer(void *p1, void *p2, void *p3)
{
	/* We need to guarantee that we receive an interrupt, so set a
	 * k_timer and spin until we die.  Spinning alone won't work
	 * on a tickless kernel.
	 */
	static struct k_timer timer;

	blow_up_stack();
	k_timer_init(&timer, NULL, NULL);
	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
	while (true) {
	}
}

void stack_sentinel_swap(void *p1, void *p2, void *p3)
{
	unsigned int key = irq_lock();

	/* Test that stack overflow check due to swap works */
	blow_up_stack();
	TC_PRINT("swapping...\n");
	z_swap_unlocked();
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

void stack_hw_overflow(void *p1, void *p2, void *p3)
{
	/* Test that HW stack overflow check works */
	blow_up_stack();
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
}

#if defined(CONFIG_USERSPACE)
void user_priv_stack_hw_overflow(void *p1, void *p2, void *p3)
{
	/* Test that HW stack overflow check works
	 * on a user thread's privilege stack.
	 */
	blow_up_priv_stack();
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
}
#endif /* CONFIG_USERSPACE */

void check_stack_overflow(k_thread_entry_t handler, uint32_t flags)
{
#ifdef CONFIG_STACK_SENTINEL
	/* When testing stack sentinel feature, the overflow stack is a
	 * smaller section of alt_stack near the end.
	 * In this way when it gets overflowed by blow_up_stack() we don't
	 * corrupt anything else and prevent the test case from completing.
	 */
	k_thread_create(&alt_thread, overflow_stack, OVERFLOW_STACKSIZE,
#else
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
#endif /* CONFIG_STACK_SENTINEL */
			handler,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), flags,
			K_NO_WAIT);

	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
}
#endif /* !CONFIG_ARCH_POSIX */

/**
 * @brief Test the kernel fatal error handling works correctly
 * @details Manually trigger the crash with various ways and check
 * that the kernel is handling that properly or not. Also the crash reason
 * should match. Check for stack sentinel feature by overflowing the
 * thread's stack and check for the exception.
 *
 * @ingroup kernel_common_tests
 */
void test_fatal(void)
{
	rv = TC_PASS;

	/*
	 * Main thread(test_main) priority was 10 but ztest thread runs at
	 * priority -1. To run the test smoothly make both main and ztest
	 * threads run at same priority level.
	 */
	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

#ifndef CONFIG_ARCH_POSIX
	TC_PRINT("test alt thread 1: generic CPU exception\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_cpu_exception,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
#else
	/*
	 * We want the native OS to handle segfaults so we can debug it
	 * with the normal linux tools
	 */
	TC_PRINT("test alt thread 1: skipped for POSIX ARCH\n");
#endif

	TC_PRINT("test alt thread 2: initiate kernel oops\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_oops,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

	TC_PRINT("test alt thread 3: initiate kernel panic\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_panic,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

#if defined(CONFIG_ASSERT)
	/* This test shall be skip while ASSERT is off */
	TC_PRINT("test alt thread 4: fail assertion\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_zephyr_assert,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
#endif

	TC_PRINT("test alt thread 5: initiate arbitrary SW exception\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			entry_arbitrary_reason,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

#ifndef CONFIG_ARCH_POSIX

#ifdef CONFIG_STACK_SENTINEL
	TC_PRINT("test stack sentinel overflow - timer irq\n");
	check_stack_overflow(stack_sentinel_timer, 0);

	TC_PRINT("test stack sentinel overflow - swap\n");
	check_stack_overflow(stack_sentinel_swap, 0);
#endif /* CONFIG_STACK_SENTINEL */

#ifdef CONFIG_HW_STACK_PROTECTION
	/* HW based stack overflow detection.
	 * Do this twice to show that HW-based solutions work more than
	 * once.
	 */

	TC_PRINT("test stack HW-based overflow - supervisor 1\n");
	check_stack_overflow(stack_hw_overflow, 0);

	TC_PRINT("test stack HW-based overflow - supervisor 2\n");
	check_stack_overflow(stack_hw_overflow, 0);

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	TC_PRINT("test stack HW-based overflow (FPU thread) - supervisor 1\n");
	check_stack_overflow(stack_hw_overflow, K_FP_REGS);

	TC_PRINT("test stack HW-based overflow (FPU thread) - supervisor 2\n");
	check_stack_overflow(stack_hw_overflow, K_FP_REGS);
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_USERSPACE

	TC_PRINT("test stack HW-based overflow - user 1\n");
	check_stack_overflow(stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user 2\n");
	check_stack_overflow(stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user priv stack 1\n");
	check_stack_overflow(user_priv_stack_hw_overflow, K_USER);

	TC_PRINT("test stack HW-based overflow - user priv stack 2\n");
	check_stack_overflow(user_priv_stack_hw_overflow, K_USER);

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	TC_PRINT("test stack HW-based overflow (FPU thread) - user 1\n");
	check_stack_overflow(stack_hw_overflow, K_USER | K_FP_REGS);

	TC_PRINT("test stack HW-based overflow (FPU thread) - user 2\n");
	check_stack_overflow(stack_hw_overflow, K_USER | K_FP_REGS);
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#endif /* CONFIG_USERSPACE */

#endif /* CONFIG_HW_STACK_PROTECTION */

#endif /* !CONFIG_ARCH_POSIX */
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(fatal,
			ztest_unit_test(test_fatal));
	ztest_run_test_suite(fatal);
}
