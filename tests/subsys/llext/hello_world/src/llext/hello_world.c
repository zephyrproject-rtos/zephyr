/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This very simple hello world C code can be used as a test case for building
 * probably the simplest loadable extension. It requires a single symbol be
 * linked, section relocation support, and the ability to export and call out to
 * a function.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/kernel.h>

static const uint32_t number = 42;

#define STACK_SIZE 1024
static K_THREAD_STACK_DEFINE(dyn_thread_stack, STACK_SIZE);
struct k_thread *dyn_thread;

/*
struct k_mutex my_mutex;
struct k_sem my_sem;
*/

void thread_test(void *unused1, void *unused2, void *unused3)
{
	printk("test thread\n");
}

/*
void method_test()
{
	printk("test method call\n");
}
*/

void hello_world(void)
{
	printk("hello world\n");
	printk("A number is %u\n", number);

	/*
	method_test();

	k_mutex_init(&my_mutex);
	k_mutex_lock(&my_mutex, K_FOREVER);
	k_mutex_unlock(&my_mutex);

	k_sem_init(&my_sem, 1, 5);
	k_sem_take(&my_sem, K_FOREVER);
	k_sem_give(&my_sem);
	*/

	dyn_thread = k_object_alloc(K_OBJ_THREAD);
	k_tid_t tid = k_thread_create(dyn_thread, dyn_thread_stack, STACK_SIZE,
				thread_test, NULL, NULL, NULL,
				0, 0, K_FOREVER);
	k_thread_start(tid);
}
LL_EXTENSION_SYMBOL(hello_world);
