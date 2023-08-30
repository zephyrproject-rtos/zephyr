/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <xtensa/corebits.h>
#include <xtensa/config/core-isa.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/syscall.h>

/* This choice of IRQ 11 is specific to sample_controller; we want a
 * software interrupt at level 2 or higher.  (Note that gen_zsr.py
 * will pick this same interrupt for irq_offload())
 */
#define SWINT 11
BUILD_ASSERT(XCHAL_INT11_TYPE == XTHAL_INTTYPE_SOFTWARE);
BUILD_ASSERT(XCHAL_INT11_LEVEL > 1);

volatile int one;
volatile int zero;
volatile int swint_ran;
volatile int timer_done;

struct k_thread boom;
K_THREAD_STACK_DEFINE(boom_stack, 2048);

struct k_timer timer;

void *_k_syscall_table[K_SYSCALL_LIMIT];

int priv_stack[512];
int *_mock_priv_stack = &priv_stack[ARRAY_SIZE(priv_stack)];

void syscall_bad(void)
{
	printk("bad syscall!\n");
}

int syscall_4(int a, int b, int c, int d)
{
	printk("syscall_4 %d:%d:%d:%d\n", a, b, c, d);
	return 99;
}

int do_syscall(int num, int a, int b, int c, int d)
{
	for (int i = 0; i < K_SYSCALL_LIMIT; i++) {
		void *f = i == num ? (void *)syscall_4 : (void *)syscall_bad;

		_k_syscall_table[i] = f;
	}
	return arch_syscall_invoke4(a, b, c, d, num);
}

void syscall_fn(void *a, void *b, void *c)
{
	printk("Starting syscall test, setting PS to user mode\n");
	XTENSA_WSR("PS", XTENSA_RSR("PS") | PS_UM_MASK);

	printk("Trapping to syscall\n");
	int x = do_syscall(3, 12, 34, 56, 78);
	printk("Syscall returned %d\n", x);

	printk("Trying invalid syscall number\n");
	do_syscall(20972, 0, 0, 0, 0);

	printk("Restoring kernel mode, retrying syscall (should oops)\n");
	XTENSA_WSR("PS", XTENSA_RSR("PS") & ~PS_UM_MASK);
	do_syscall(3, 12, 34, 56, 78);

	printk("(should not reach this line)\n");
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
	printk("fatal error!\n");
}

void assert_post_action(const char *f, unsigned int l) { while(1); }

void boom_fn(void *a, void *b, void *c)
{
	one = 1;
	printk("Dividing by zero:\n");
	int x = one/zero;
	printk("1/0 = %d\n", x);
}

void swint_isr(void *arg)
{
	printk("swint! (nested = %d)\n", arch_curr_cpu()->nested);
	swint_ran = 1;
}

void trigger_swint(void)
{
	int val = BIT(SWINT);

	swint_ran = 0;
	__asm__ volatile("wsr %0, INTSET" :: "r"(val));
}

void timer_fn(struct k_timer *t)
{
	printk("Timer context; triggering nested interrupt...\n");
	trigger_swint();
	while(!swint_ran);
	printk("Timer done\n");
	timer_done = 1;
}

int main(void)
{
	IRQ_CONNECT(SWINT, 0, swint_isr, NULL, 0);
	irq_enable(SWINT);

	printk("Hello World! %s\n", CONFIG_BOARD);

	printk("In main thread %p\n", k_current_get());
	k_thread_create(&boom, boom_stack, K_THREAD_STACK_SIZEOF(boom_stack),
			boom_fn, NULL, NULL, NULL,
			1, 0, K_FOREVER);

	printk("Launching crashing thread %p\n", &boom);
	k_thread_start(&boom);
	printk("Joining crashing thread\n");
	k_thread_join(&boom, K_FOREVER);

	printk("Back in main thread, sleeping for one second...\n");
	k_msleep(1000);
	printk("Still here!\n");

	printk("Flagging medium priority interrupt\n");
	trigger_swint();
	printk("flagged, back\n");

	printk("Triggering nested swint from timer\n");
	k_timer_init(&timer, timer_fn, NULL);
	k_timer_start(&timer, K_MSEC(1), K_FOREVER);
	while(!timer_done);

	printk("Launching syscall thread %p\n", &boom);
	k_thread_create(&boom, boom_stack, K_THREAD_STACK_SIZEOF(boom_stack),
			syscall_fn, NULL, NULL, NULL,
			1, 0, K_NO_WAIT);
	k_thread_join(&boom, K_FOREVER);

	printk("Back in main thread, end of test.\n");
	return 0;
}
