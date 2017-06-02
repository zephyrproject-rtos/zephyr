/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <kernel_structs.h>
#include <irq_offload.h>

#define STACKSIZE 2048
#define MAIN_PRIORITY 7
#define PRIORITY 5

static K_THREAD_STACK_DEFINE(alt_stack, STACKSIZE);

#ifdef CONFIG_STACK_SENTINEL
#define OVERFLOW_STACKSIZE 1024
static char *overflow_stack = alt_stack + (STACKSIZE - OVERFLOW_STACKSIZE);
#else
#define OVERFLOW_STACKSIZE STACKSIZE
#endif

static struct k_thread alt_thread;
volatile int rv;

static volatile int crash_reason;

/* ARM is a special case, in that k_thread_abort() does indeed return
 * instead of calling _Swap() directly. The PendSV exception is queued
 * and immediately fires upon completing the exception path; the faulting
 * thread is never run again.
 */
#ifndef CONFIG_ARM
FUNC_NORETURN
#endif
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	TC_PRINT("Caught system error -- reason %d\n", reason);
	crash_reason = reason;

	k_thread_abort(_current);
#ifndef CONFIG_ARM
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
	int key;

	key = irq_lock();
	k_oops();
	TC_ERROR("SHOULD NEVER SEE THIS\n");
	rv = TC_FAIL;
	irq_unlock(key);
}

void alt_thread3(void)
{
	int key;

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
	memset(buf, 0xbb, sizeof(buf));
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
	int key = irq_lock();

	/* Test that stack overflow check due to swap works */
	blow_up_stack();
	TC_PRINT("swapping...\n");
	_Swap(irq_lock());
	TC_ERROR("should never see this\n");
	rv = TC_FAIL;
	irq_unlock(key);
}


void main(void)
{
	rv = TC_PASS;

	TC_START("test_fatal");

	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

	TC_PRINT("test alt thread 1: generic CPU exception\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			(k_thread_entry_t)alt_thread1,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	if (rv == TC_FAIL) {
		TC_ERROR("thread was not aborted\n");
		goto out;
	} else {
		TC_PRINT("PASS\n");
	}

	TC_PRINT("test alt thread 2: initiate kernel oops\n");
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			(k_thread_entry_t)alt_thread2,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	if (crash_reason != _NANO_ERR_KERNEL_OOPS) {
		TC_ERROR("bad reason code got %d expected %d\n",
			 crash_reason, _NANO_ERR_KERNEL_OOPS);
		rv = TC_FAIL;
	}
	if (rv == TC_FAIL) {
		TC_ERROR("thread was not aborted\n");
		goto out;
	} else {
		TC_PRINT("PASS\n");
	}

	TC_PRINT("test alt thread 3: initiate kernel panic\n");
	k_thread_create(&alt_thread, alt_stack, sizeof(alt_stack),
			(k_thread_entry_t)alt_thread3,
			NULL, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
	k_thread_abort(&alt_thread);
	if (crash_reason != _NANO_ERR_KERNEL_PANIC) {
		TC_ERROR("bad reason code got %d expected %d\n",
			 crash_reason, _NANO_ERR_KERNEL_PANIC);
		rv = TC_FAIL;
	}
	if (rv == TC_FAIL) {
		TC_ERROR("thread was not aborted\n");
		goto out;
	} else {
		TC_PRINT("PASS\n");
	}

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
	if (crash_reason != _NANO_ERR_STACK_CHK_FAIL) {
		TC_ERROR("bad reason code got %d expected %d\n",
			 crash_reason, _NANO_ERR_KERNEL_PANIC);
		rv = TC_FAIL;
	}
	if (rv == TC_FAIL) {
		TC_ERROR("thread was not aborted\n");
		goto out;
	} else {
		TC_PRINT("PASS\n");
	}

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
	if (crash_reason != _NANO_ERR_STACK_CHK_FAIL) {
		TC_ERROR("bad reason code got %d expected %d\n",
			 crash_reason, _NANO_ERR_KERNEL_PANIC);
		rv = TC_FAIL;
	}
	if (rv == TC_FAIL) {
		TC_ERROR("thread was not aborted\n");
		goto out;
	} else {
		TC_PRINT("PASS\n");
	}
out:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
