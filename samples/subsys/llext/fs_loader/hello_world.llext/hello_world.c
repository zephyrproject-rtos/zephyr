/**
 * Copyright (c) 2024 Schneider Electric
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/llext/symbol.h>


#include <zephyr/logging/log.h>
/* In all files comprising the module but one */
LOG_MODULE_DECLARE(extension, LOG_LEVEL_INF);


/* semaphore to end extension thread */
struct k_sem sem_end_thread;


static void extension_thread(void *p1, void *p2, void *p3)
{
	LOG_INF("llext thread: start");

	while (true) {
		int ret = k_sem_take(&sem_end_thread, K_MSEC(1000));
		if (ret != 0) {
			/* timeout */
			k_tid_t thread_id = k_sched_current_thread_query();
			printk("Hello World from extension thread %p\n", thread_id);
		} else {
			/* quit thread */
			break;
		}
	}

	LOG_INF("llext thread: end");
}

#define STACK_SIZE 2048
#define PRIORITY   5

K_THREAD_STACK_DEFINE(hello_world_stack, STACK_SIZE);
struct k_thread new_thread;

void start(void)
{
	LOG_INF("Starting extension...");

	k_sem_init(&sem_end_thread, 0, 10);

	k_tid_t tid = k_thread_create(&new_thread, hello_world_stack, STACK_SIZE,
								  extension_thread, NULL, NULL, NULL, PRIORITY,
								  K_ESSENTIAL, K_MSEC(0));
#ifdef CONFIG_THREAD_NAME
	strncpy(new_thread.name, "extension", CONFIG_THREAD_MAX_NAME_LEN - 1);
	/* Ensure NULL termination, truncate if longer */
	new_thread.name[CONFIG_THREAD_MAX_NAME_LEN - 1] = '\0';

#endif

	LOG_INF("Extension Started tid=%p", tid);
}

void stop(void)
{
	k_sem_give(&sem_end_thread);
}

LL_EXTENSION_SYMBOL(start);
LL_EXTENSION_SYMBOL(stop);
