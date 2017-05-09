/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <kernel_structs.h>
#include <irq_offload.h>

#define STACKSIZE 1024
#define PRIORITY 5

static char __noinit __stack alt_stack[STACKSIZE];
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
	k_oops();
	rv = TC_FAIL;
}


void main(void)
{
	rv = TC_PASS;

	TC_START("test_fatal");

	printk("test alt thread 1: generic CPU exception\n");
	k_thread_create(&alt_thread, alt_stack, STACKSIZE,
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
	k_thread_create(&alt_thread, alt_stack, STACKSIZE,
			(k_thread_entry_t)alt_thread2,
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
