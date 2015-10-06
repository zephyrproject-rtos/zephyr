/* nanokernel_footprint.c - nanokernel footprint */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

#include <nanokernel.h>
#include <toolchain.h>

volatile int i = 0;		/* counter used by background task */

#ifndef TEST_min

#include <misc/printk.h>
#include <arch/cpu.h>
#include <stdio.h>

#ifdef TEST_reg
#if defined(__GNUC__)
#include <test_asm_inline_gcc.h>
#else
#include <test_asm_inline_other.h>
#endif /* __GNUC__ */
#endif /* TEST_reg */

#define IRQ_PRIORITY      3
#define TEST_SOFT_INT	  64

#ifdef TEST_max
#define FIBER_STACK_SIZE  1024
#else
#define FIBER_STACK_SIZE  512
#endif /* TEST_max */

#ifdef TEST_max
#define MESSAGE "Running maximal nanokernel configuration\n"
#else
#define MESSAGE "Running regular nanokernel configuration\n"
#endif /* TEST_max */

typedef void* (*pfunc) (void*);

/* variables */

/* ISR stub data structure */
#ifndef TEST_max
static void isrDummyIntStub(void *);
NANO_CPU_INT_REGISTER(isrDummyIntStub, -1, -1, TEST_SOFT_INT, 0);
#endif /* TEST_max */

/* stack used by fiber */
static char __stack pStack[FIBER_STACK_SIZE];

/* pointer array ensures specified functions are linked into the image */
volatile pfunc func_array[] = {
	/* nano timer functions */
	(pfunc)nano_timer_init,
	(pfunc)nano_fiber_timer_start,
	(pfunc)nano_fiber_timer_wait,
	/* nano semaphore functions */
	(pfunc)nano_sem_init,
	(pfunc)nano_fiber_sem_take_wait,
	(pfunc)nano_fiber_sem_give,
#ifdef TEST_max
	/* nano LIFO functions */
	(pfunc)nano_lifo_init,
	(pfunc)nano_fiber_lifo_put,
	(pfunc)nano_fiber_lifo_get,
	/* nano stack functions */
	(pfunc)nano_stack_init,
	(pfunc)nano_fiber_stack_push,
	(pfunc)nano_fiber_stack_pop,
	/* nano FIFO functions */
	(pfunc)nano_fifo_init,
	(pfunc)nano_fiber_fifo_put,
	(pfunc)nano_fiber_fifo_get,
#endif /* TEST_max */
	};

/**
 *
 * @brief Dummy ISR
 *
 * @return N/A
 */

void dummyIsr(void *unused)
{
	ARG_UNUSED(unused);
}

#ifdef TEST_reg
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
#endif /* TEST_reg */

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

#ifdef TEST_max
	printf((char *)message);
#else
	printk((char *)message);
#endif /* TEST_max */
}

#endif /* !TEST_min */

/**
 *
 * @brief Mainline for background task
 *
 * This routine simply increments a global counter.
 * (Gdb can be used to observe the counter as it increases.)
 *
 * @return N/A
 */

void main(void)
{
#ifdef TEST_max
	/* dynamically link in dummy ISR */
	irq_connect(NANO_SOFT_IRQ, IRQ_PRIORITY, dummyIsr,
				(void *) 0);
#endif /* TEST_max */
#ifndef TEST_min
	/* start a trivial fiber */
	task_fiber_start(pStack, FIBER_STACK_SIZE, fiberEntry, (int) MESSAGE,
					 (int) func_array, 10, 0);
#endif /* !TEST_min */

	while (1) {
		i++;
	}
}
