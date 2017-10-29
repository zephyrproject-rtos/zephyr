/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <toolchain.h>


#include <misc/printk.h>
#include <stdio.h>

#ifdef CONFIG_OBJECTS_WHILELOOP
volatile int i;
#endif

#define IRQ_LINE          10
#define IRQ_PRIORITY      3
#define TEST_SOFT_INT     64
#define TEST_IRQ_OFFLOAD_VECTOR 32

#define THREAD_STACK_SIZE  CONFIG_THREAD_STACK_SIZE

typedef void * (*pfunc) (void *);

/* variables */

#define MESSAGE "Running maximal kernel configuration\n"

/* stack used by thread */
#ifdef CONFIG_OBJECTS_THREAD
static K_THREAD_STACK_DEFINE(pStack, THREAD_STACK_SIZE);
static struct k_thread objects_thread;
#endif

/* pointer array ensures specified functions are linked into the image */
volatile pfunc func_array[] = {
	/* timer functions */
#ifdef CONFIG_OBJECTS_TIMER
	(pfunc)k_timer_init,
	(pfunc)k_timer_stop,
	(pfunc)k_timer_status_get,
	(pfunc)k_timer_status_sync,
	(pfunc)k_timer_remaining_get,
	(pfunc)k_uptime_get,
	(pfunc)k_uptime_get_32,
	(pfunc)k_uptime_delta,
	(pfunc)k_uptime_delta_32,
#endif

	/* semaphore functions */
#ifdef CONFIG_OBJECTS_SEMAPHORE
	(pfunc)k_sem_init,
	(pfunc)k_sem_take,
	(pfunc)k_sem_give,
	(pfunc)k_sem_reset,
	(pfunc)k_sem_count_get,
#endif

	/* LIFO functions */
#ifdef CONFIG_OBJECTS_LIFO
	(pfunc)k_queue_prepend,
	(pfunc)k_queue_init,
	(pfunc)k_queue_get,
#endif
	/* stack functions */
#ifdef CONFIG_OBJECTS_STACK
	(pfunc)k_stack_init,
	(pfunc)k_stack_push,
	(pfunc)k_stack_pop,
#endif
	/* FIFO functions */
#ifdef CONFIG_OBJECTS_FIFO
	(pfunc)k_queue_prepend,
	(pfunc)k_queue_init,
	(pfunc)k_queue_get,
#endif

};

void dummy_isr(void *unused)
{
	ARG_UNUSED(unused);
}

#ifdef CONFIG_OBJECTS_THREAD

/**
 *
 * @brief Trivial thread
 *
 * @param message   Message to be printed.
 * @param arg1	Unused.
 *
 * @return N/A
 */
static void thread_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	printk("%s\n", (char *)arg1);
}
#endif



void main(void)
{
#ifdef CONFIG_OBJECTS_PRINTK
	printk("Using printk\n");
#endif

#if CONFIG_STATIC_ISR
	IRQ_CONNECT(IRQ_LINE, IRQ_PRIORITY, dummy_isr, NULL, 0);
#endif

#ifdef CONFIG_OBJECTS_THREAD
	/* start a trivial thread */
	k_thread_create(&objects_thread, pStack, THREAD_STACK_SIZE,
			thread_entry, MESSAGE, (void *)func_array,
			NULL, 10, 0, K_NO_WAIT);
#endif

#ifdef CONFIG_OBJECTS_WHILELOOP
	i = 0;
	while (1) {
		i++;
	}
#endif
}
