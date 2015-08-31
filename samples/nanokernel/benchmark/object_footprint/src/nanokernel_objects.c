/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nanokernel.h>
#include <toolchain.h>


#include <misc/printk.h>
#include <arch/cpu.h>
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
				(void *) 0);
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
