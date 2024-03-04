/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>

#include <app_api.h>

#define STACKSIZE 512
#define PRIORITY 2

static struct k_sem *my_sem;

static void tick_sub(void *p1, void *p2, void *p3)
{
	struct k_event *tick_evt = k_object_alloc(K_OBJ_EVENT);

	k_event_init(tick_evt);

	register_subscriber(CHAN_TICK, tick_evt);

	while (true) {
		printk("[ext3]Waiting event\n");
		k_event_wait(tick_evt, CHAN_TICK, true, K_FOREVER);
		printk("[ext3]Got event, giving sem\n");

		k_sem_give(my_sem);
	}
}

int start(void)
{
	k_thread_stack_t *sub_stack;
	struct k_thread *sub_thread;

	/* Currently, any kobjects need to be dynamic on extensions,
	 * so the semaphore, thread stack and thread objects are created
	 * dynamically.
	 */
	my_sem = k_object_alloc(K_OBJ_SEM);
	k_sem_init(my_sem, 0, 1);

	sub_stack = k_thread_stack_alloc(STACKSIZE, K_USER);
	sub_thread = k_object_alloc(K_OBJ_THREAD);
	printk("[ext3]%p - %p\n", sub_stack, sub_thread);
	k_thread_create(sub_thread, sub_stack, STACKSIZE, tick_sub, NULL, NULL,
			NULL, PRIORITY, K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	while (true) {
		long l;

		printk("[ext3]Waiting sem\n");
		k_sem_take(my_sem, K_FOREVER);

		printk("[ext3]Got sem, reading channel\n");
		receive(CHAN_TICK, &l, sizeof(l));
		printk("[ext3]Read val: %ld\n", l);
	}

	return 0;
}
LL_EXTENSION_SYMBOL(start);
