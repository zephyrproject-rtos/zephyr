/* nanokernel_footprint.c - nanokernel footprint */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

volatile int i = 0;		/* counter used by background task */

#ifndef TEST_min

/* includes */

#include <misc/printk.h>
#include <nanokernel/cpu.h>
#include <stdio.h>

#ifdef TEST_reg
#if defined(__GNUC__)
#include <test_asm_inline_gcc.h>
#else
#include <test_asm_inline_other.h>
#endif /* __GNUC__ */
#endif /* TEST_reg */

/* defines */

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

/* typedefs */

typedef void* (*pfunc) (void*);

/* variables */

/* ISR stub data structure */
#ifdef TEST_max
static NANO_CPU_INT_STUB_DECL(isrDummyHandlerStub);
#else
static void isrDummyIntStub(void *);
NANO_CPU_INT_REGISTER(isrDummyIntStub, TEST_SOFT_INT, 0);
#endif /* TEST_max */

/* stack used by fiber */
static char pStack[FIBER_STACK_SIZE];

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

/*******************************************************************************
 *
 * dummyIsr - dummy ISR
 *
 * RETURNS: N/A
 */

void dummyIsr(void *unused)
	{
	ARG_UNUSED(unused);
	}

#ifdef TEST_reg
/*******************************************************************************
 *
 * isrDummyIntStub - static interrupt stub that invokes dummy ISR
 *
 * NOTE: This is typically coded in assembly language, rather than C,
 * to avoid the preamble code the compiler automatically generates. However,
 * the unwanted preamble has an insignificant impact on total footprint.
 *
 * RETURNS: N/A
 */

static void isrDummyIntStub(void *unused)
	{
	ARG_UNUSED(unused);

	isr_dummy();

	CODE_UNREACHABLE;
	}
#endif /* TEST_reg */

/*******************************************************************************
 *
 * fiberEntry - trivial fiber
 *
 * RETURNS: N/A
 */

static void fiberEntry
	(
	int  message,	/* message to be printed */
	int  arg1		/* unused */
	)
	{
	ARG_UNUSED(arg1);

#ifdef TEST_max
	printf((char *)message);
#else
	printk((char *)message);
#endif /* TEST_max */
	}

#endif /* !TEST_min */

/*******************************************************************************
 *
 * main - mainline for background task
 *
 * This routine simply increments a global counter.
 * (Gdb can be used to observe the counter as it increases.)
 *
 * RETURNS: N/A
 */

void main(void)
	{
#ifdef TEST_max
    /* dynamically link in dummy ISR */
	irq_connect(NANO_SOFT_IRQ, IRQ_PRIORITY, dummyIsr,
		       (void *) 0, isrDummyHandlerStub);
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
