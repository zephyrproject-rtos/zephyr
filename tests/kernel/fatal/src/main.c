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

static char __noinit __stack alt_stack[STACKSIZE];

#ifdef CONFIG_STACK_SENTINEL
#define OVERFLOW_STACKSIZE 1024
static char *overflow_stack = alt_stack + (STACKSIZE - OVERFLOW_STACKSIZE);
#else
#define OVERFLOW_STACKSIZE STACKSIZE
#endif

static struct k_thread alt_thread;
volatile int rv;

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
	rv = TC_FAIL;
	irq_unlock(key);
}


void blow_up_stack(void)
{
	char buf[OVERFLOW_STACKSIZE];
	printk("posting %zu bytes of junk to stack...\n", sizeof(buf));
	memset(buf, 0xbb, sizeof(buf));
}

void stack_thread1(void)
{
	/* Test that stack overflow check due to timer interrupt works */
	blow_up_stack();
	printk("busy waiting...\n");
	k_busy_wait(1024 * 1024);
	printk("should never see this\n");
	rv = TC_FAIL;
}


void stack_thread2(void)
{
	int key = irq_lock();

	/* Test that stack overflow check due to swap works */
	blow_up_stack();
	printk("swapping...\n");
	_Swap(irq_lock());
	printk("should never see this\n");
	rv = TC_FAIL;
	irq_unlock(key);
}


void main(void)
{
	rv = TC_PASS;

	TC_START("test_fatal");

	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

	printk("test alt thread 1: generic CPU exception\n");
	k_thread_create(&alt_thread, alt_stack, sizeof(alt_stack),
			(k_thread_entry_t)alt_thread1,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), 0,
			K_NO_WAIT);
	if (rv == TC_FAIL) {
		printk("thread was not aborted\n");
		goto out;
	} else {
		printk("PASS\n");
	}

	printk("test alt thread 2: initiate kernel oops\n");
	k_thread_create(&alt_thread, alt_stack, sizeof(alt_stack),
			(k_thread_entry_t)alt_thread2,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), 0,
			K_NO_WAIT);
	if (rv == TC_FAIL) {
		printk("thread was not aborted\n");
		goto out;
	} else {
		printk("PASS\n");
	}

	printk("test stack overflow - timer irq\n");
#ifdef CONFIG_STACK_SENTINEL
	/* When testing stack sentinel feature, the overflow stack is a
	 * smaller section of alt_stack near the end.
	 * In this way when it gets overflowed by blow_up_stack() we don't
	 * corrupt anything else and prevent the test case from completing.
	 */
	k_thread_create(&alt_thread, overflow_stack, OVERFLOW_STACKSIZE,
#else
	k_thread_create(&alt_thread, alt_stack, sizeof(alt_stack),
#endif
			(k_thread_entry_t)stack_thread1,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), 0,
			K_NO_WAIT);
	if (rv == TC_FAIL) {
		printk("thread was not aborted\n");
		goto out;
	} else {
		printk("PASS\n");
	}

	printk("test stack overflow - swap\n");
#ifdef CONFIG_STACK_SENTINEL
	k_thread_create(&alt_thread, overflow_stack, OVERFLOW_STACKSIZE,
#else
	k_thread_create(&alt_thread, alt_stack, sizeof(alt_stack),
#endif
			(k_thread_entry_t)stack_thread2,
			NULL, NULL, NULL, K_PRIO_PREEMPT(PRIORITY), 0,
			K_NO_WAIT);
	if (rv == TC_FAIL) {
		printk("thread was not aborted\n");
		goto out;
	} else {
		printk("PASS\n");
	}

out:
	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
