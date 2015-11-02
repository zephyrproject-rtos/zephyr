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

#if defined(__GNUC__)
#include <test_asm_inline_gcc.h>
#else
#include <test_asm_inline_other.h>
#endif /* __GNUC__ */

#define IRQ_PRIORITY      3
#define TEST_SOFT_INT	  64

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
	(pfunc)nano_fiber_timer_wait,
#endif

	/* nano semaphore functions */
#ifdef CONFIG_OBJECTS_SEMAPHORE
	(pfunc)nano_sem_init,
	(pfunc)nano_fiber_sem_take_wait,
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

#ifdef CONFIG_STATIC_ISR
/* ISR stub data structure */
static void isrDummyIntStub(void *);
NANO_CPU_INT_REGISTER(isrDummyIntStub, TEST_SOFT_INT, 0);
#endif

void dummyIsr(void *unused)
{
	ARG_UNUSED(unused);
}

#ifdef CONFIG_STATIC_ISR
/**
 *
 * @brief Static interrupt stub that invokes dummy ISR
 *
 * NOTE: This is typically coded in assembly language, rather than C,
 * to avoid the preamble code the compiler automatically generates. However,
 * the unwanted preamble has an insignificant impact on total footprint.
 *
 * @return N/A
 */
static void isrDummyIntStub(void *unused)
{
	ARG_UNUSED(unused);

	isr_dummy();

	CODE_UNREACHABLE;
}

#endif


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

#ifdef CONFIG_DYNAMIC_ISR
	/* dynamically link in dummy ISR */
	irq_connect(NANO_SOFT_IRQ, IRQ_PRIORITY, dummyIsr,
		    (void *) 0, 0);
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
