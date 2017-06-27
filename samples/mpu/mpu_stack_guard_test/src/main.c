/* main.c - Hello World demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

/* size of stack area used by each thread */
#define STACKSIZE  512

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between greetings (in ms) */
#define SLEEPTIME 500

/* delay between greetings (in ms) */
#define SCHEDTIME 30

/* Focaccia tastes better than DEEDBEEF ;-) */
#define STACK_GUARD_CANARY 0xF0CACC1A

struct stack_guard_buffer {
	/* Make sure canary is not optimized by the compiler */
	struct k_thread thread;
	volatile u32_t canary;
	K_THREAD_STACK_MEMBER(stack, STACKSIZE);
};

#define LAST_THREAD_NUM 6
struct stack_guard_buffer buf[LAST_THREAD_NUM+1];

u32_t recursive_loop(u32_t counter, int num, void *dummy)
{

	if (buf[num].canary != STACK_GUARD_CANARY) {
		printk("Canary = 0x%08x\tTest not passed.\n", buf[num].canary);

		while (1) {
			k_sleep(SLEEPTIME);
		}
	}
	counter++;
	if (dummy == 0)
		return counter;
	return recursive_loop(counter, num, dummy)
		+recursive_loop(counter, num, dummy);
}


/* stack_guard_thread is a dynamic thread */
void stack_guard_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	recursive_loop(0, (int)dummy1, dummy2);

}

void main(void)
{
	int i;
	bool failed;

	printk("STACK_ALIGN 0x%x\n", STACK_ALIGN);
	for (i = 0; i <= LAST_THREAD_NUM; i++) {
		buf[i].canary = STACK_GUARD_CANARY;
		failed = (i == LAST_THREAD_NUM) ? true : false;
		if (failed) {
			printk("MPU STACK GUARD Test\n");
			printk("Canary Initial Value = 0x%x threads %p\n",
					buf[i].canary, &buf[i].thread);
		}

		/* create stack_guard_thread */
		k_thread_create(&buf[i].thread, buf[i].stack,
				K_THREAD_STACK_SIZEOF(buf[i].stack),
				stack_guard_thread, (void *)i,
				(void *)failed, NULL, PRIORITY, 0, K_NO_WAIT);
	}

}
