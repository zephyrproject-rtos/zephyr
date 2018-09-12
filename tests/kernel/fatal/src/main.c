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

#if defined(CONFIG_X86) && defined(CONFIG_X86_MMU)
#define STACKSIZE (8192)
#else
#define  STACKSIZE (2048)
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

static volatile int crash_reason;

/* On some architectures, k_thread_abort(_current) will return instead
 * of _Swap'ing away.
 *
 * On ARM the PendSV exception is queued and immediately fires upon
 * completing the exception path; the faulting thread is never run
 * again.
 *
 * On Xtensa/asm2 the handler is running in interrupt context and on
 * the interrupt stack and needs to return through the interrupt exit
 * code.
 *
 * In both cases the thread is guaranteed never to run again once we
 * return from the _SysFatalErrorHandler().
 */
#if !(defined(CONFIG_ARM) || defined(CONFIG_XTENSA_ASM2) || defined(CONFIG_ARC))
#define ERR_IS_NORETURN 1
#endif

#ifdef ERR_IS_NORETURN
FUNC_NORETURN
#endif
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	TC_PRINT("Caught system error -- reason %d\n", reason);
	crash_reason = reason;

	k_thread_abort(_current);
#ifdef ERR_IS_NORETURN
	CODE_UNREACHABLE;
#endif
}

void alt_thread1(void)
{
#if defined(CONFIG_X86)
	__asm__ volatile ("ud2");
#elif defined(CONFIG_NIOS2)
	__asm__ volatile ("trap");
#elif defined(CONFIG_ARC)
	__asm__ volatile ("swi");
#else
	/* Triggers usage fault on ARM, illegal instruction on RISCV32
	 * and xtensa
	 */
	{
		int illegal = 0;
		((void(*)(void))&illegal)();
	}
#endif
	rv = TC_FAIL;
}


void alt_thread2(void)
{
	unsigned int key;

	key = irq_lock();
	k_oops();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

void alt_thread3(void)
{
	unsigned int key;

	key = irq_lock();
	k_panic();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

void blow_up_stack(void)
{
	char buf[OVERFLOW_STACKSIZE];

	TC_PRINT("posting %zu bytes of junk to stack...\n", sizeof(buf));
	(void)memset(buf, 0xbb, sizeof(buf));
}

void stack_thread1(void)
{
	/* Test that stack overflow check due to timer interrupt works */
	blow_up_stack();
	TC_PRINT("busy waiting...\n");
	k_busy_wait(1024 * 1024);
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
}


void stack_thread2(void)
{
	unsigned int key = irq_lock();

	/* Test that stack overflow check due to swap works */
	blow_up_stack();
	TC_PRINT("swapping...\n");
	_Swap(irq_lock());
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

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
			(k_thread_entry_t)alt_thread1,
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
			(k_thread_entry_t)alt_thread2,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_equal(crash_reason, _NANO_ERR_KERNEL_OOPS,
		      "bad reason code got %d expected %d\n",
		      crash_reason, _NANO_ERR_KERNEL_OOPS);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

	TC_PRINT("test alt thread 3: initiate kernel panic\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			(k_thread_entry_t)alt_thread3,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	zassert_equal(crash_reason, _NANO_ERR_KERNEL_PANIC,
		      "bad reason code got %d expected %d\n",
		      crash_reason, _NANO_ERR_KERNEL_PANIC);
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

#ifndef CONFIG_ARCH_POSIX
	TC_PRINT("test stack overflow - timer irq\n");
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
#endif
			(k_thread_entry_t)stack_thread1,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), 0,
			K_NO_WAIT);

#ifdef CONFIG_ARM
	/* FIXME: See #7706 */
	zassert_true(crash_reason == _NANO_ERR_STACK_CHK_FAIL ||
		     crash_reason == _NANO_ERR_HW_EXCEPTION, NULL);
#else
	zassert_equal(crash_reason, _NANO_ERR_STACK_CHK_FAIL,
		      "bad reason code got %d expected %d\n",
		      crash_reason, _NANO_ERR_STACK_CHK_FAIL);
#endif
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");

	/* Stack sentinel has to be invoked, make sure it happens during
	 * a context switch. Also ensure HW-based solutions can run more
	 * than once.
	 */
	TC_PRINT("test stack overflow - swap\n");
#ifdef CONFIG_STACK_SENTINEL
	k_thread_create(&alt_thread, overflow_stack, OVERFLOW_STACKSIZE,
#else
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
#endif
			(k_thread_entry_t)stack_thread2,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), 0,
			K_NO_WAIT);
#ifdef CONFIG_NXP_MPU
	/* FIXME: See #7706 */
	zassert_true(crash_reason == _NANO_ERR_STACK_CHK_FAIL ||
		     crash_reason == _NANO_ERR_HW_EXCEPTION, NULL);
#else
	zassert_equal(crash_reason, _NANO_ERR_STACK_CHK_FAIL,
		      "bad reason code got %d expected %d\n",
		      crash_reason, _NANO_ERR_STACK_CHK_FAIL);
#endif
	zassert_not_equal(rv, TC_FAIL, "thread was not aborted");
#else
	TC_PRINT("test stack overflow - skipped for POSIX arch\n");
	/*
	 * We do not have a stack check for the posix ARCH
	 * again we relay on the native OS
	 */
#endif
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(fatal,
			ztest_unit_test(test_fatal));
	ztest_run_test_suite(fatal);
}
