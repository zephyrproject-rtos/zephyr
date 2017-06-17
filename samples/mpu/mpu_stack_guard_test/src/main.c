/* main.c - Hello World demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between greetings (in ms) */
#define SLEEPTIME 500

/* Focaccia tastes better than DEEDBEEF ;-) */
#define STACK_GUARD_CANARY 0xF0CACC1A

struct stack_guard_buffer {
	/* Make sure canary is not optimized by the compiler */
	volatile u32_t canary;
	u8_t stack[STACKSIZE];
};

struct stack_guard_buffer buf;
struct k_thread test_thread;

u32_t recursive_loop(u32_t counter)
{
	if (buf.canary != STACK_GUARD_CANARY) {
		printk("Canary = 0x%08x\tTest not passed.\n", buf.canary);

		while (1) {
			k_sleep(SLEEPTIME);
		}
	}

	counter++;

	return recursive_loop(counter) + recursive_loop(counter);
}

/* stack_guard_thread is a dynamic thread */
void stack_guard_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	u32_t result = recursive_loop(0);

	/* We will never arrive here */
	printk("Result: %d", result);
}

void main(void)
{
	buf.canary = STACK_GUARD_CANARY;

	printk("MPU STACK GUARD Test\n");
	printk("Canary Initial Value = 0x%x\n", buf.canary);

	/* spawn stack_guard_thread */
	k_thread_create(&test_thread, buf.stack, STACKSIZE, stack_guard_thread,
			NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
}
