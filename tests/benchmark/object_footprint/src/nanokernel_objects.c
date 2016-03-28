/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <toolchain.h>


#include <misc/printk.h>
#include <stdio.h>

#ifdef CONFIG_OBJECTS_WHILELOOP
volatile int i = 0;
#endif

#define IRQ_LINE          10
#define IRQ_PRIORITY      3
#define TEST_SOFT_INT	  64
#define TEST_IRQ_OFFLOAD_VECTOR	32

#define FIBER_STACK_SIZE  CONFIG_FIBER_STACK_SIZE

typedef void* (*pfunc) (void*);

/* variables */

#define MESSAGE "Running maximal nanokernel configuration\n"

/* stack used by fiber */
#ifdef CONFIG_OBJECTS_FIBER
static char __stack pStack[FIBER_STACK_SIZE];
#endif

/* pointer array ensures specified functions are linked into the image */
volatile pfunc func_array[] = {
	/* nano timer functions */
#ifdef CONFIG_OBJECTS_TIMER
	(pfunc)nano_timer_init,
	(pfunc)nano_fiber_timer_start,
	(pfunc)nano_fiber_timer_test,
#endif

	/* nano semaphore functions */
#ifdef CONFIG_OBJECTS_SEMAPHORE
	(pfunc)nano_sem_init,
	(pfunc)nano_fiber_sem_take,
	(pfunc)nano_fiber_sem_give,
#endif

	/* nano LIFO functions */
#ifdef CONFIG_OBJECTS_LIFO
	(pfunc)nano_lifo_init,
	(pfunc)nano_fiber_lifo_put,
	(pfunc)nano_fiber_lifo_get,
#endif
	/* nano stack functions */
#ifdef CONFIG_OBJECTS_STACK
	(pfunc)nano_stack_init,
	(pfunc)nano_fiber_stack_push,
	(pfunc)nano_fiber_stack_pop,
#endif
	/* nano FIFO functions */
#ifdef CONFIG_OBJECTS_FIFO
	(pfunc)nano_fifo_init,
	(pfunc)nano_fiber_fifo_put,
	(pfunc)nano_fiber_fifo_get,
#endif

};

void dummyIsr(void *unused)
{
	ARG_UNUSED(unused);
}

#ifdef CONFIG_OBJECTS_FIBER

/**
 *
 * @brief Trivial fiber
 *
 * @param message   Message to be printed.
 * @param arg1		Unused.
 *
 * @return N/A
 */
static void fiberEntry(int message,	int arg1)
{
	ARG_UNUSED(arg1);
	printk((char *)message);
}
#endif



void main(void)
{
#ifdef CONFIG_OBJECTS_PRINTK
	printk("Using printk\n");
#endif

#if CONFIG_STATIC_ISR
	IRQ_CONNECT(IRQ_LINE, IRQ_PRIORITY, dummyIsr, NULL, 0);
#endif

#ifdef CONFIG_OBJECTS_FIBER
	/* start a trivial fiber */
	task_fiber_start(pStack, FIBER_STACK_SIZE, fiberEntry, (int) MESSAGE,
					 (int) func_array, 10, 0);
#endif

#ifdef CONFIG_OBJECTS_WHILELOOP
	while (1) {
		i++;
	}
#endif
}
