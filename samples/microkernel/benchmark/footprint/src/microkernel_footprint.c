/* microkernel_footprint.c - microkernel footprint */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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

/* includes */

#include <microkernel.h>
#include <toolchain.h>

#ifdef TEST_min

/* INTENTIONALLY LEFT EMPTY (I.E. NO APPLICATION TASKS, FIBERS, OR ISRS) */

#else

/* includes */

#include <nanokernel/cpu.h>
#include <misc/printk.h>
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
#define MESSAGE "Running maximal microkernel configuration\n"
#else
#define MESSAGE "Running regular microkernel configuration\n"
#endif /* TEST_max */

/* typedefs */

typedef void* (*pfunc) (void*);

/* variables */

volatile int i = 0;		/* counter used by foreground task */

/* ISR stub data structure */
#ifdef TEST_max
static NANO_CPU_INT_STUB_DECL(isrDummyHandlerStub);
#else
static void isrDummyIntStub (void *);
NANO_CPU_INT_REGISTER (isrDummyIntStub, TEST_SOFT_INT, 0);
#endif /* TEST_max */

/* pointer array ensures specified functions are linked into the image */
static pfunc func_array[] = {
    /* event functions */
	(pfunc)task_event_send,
	(pfunc)_task_event_recv,
    /* mutex functions */
	(pfunc)_task_mutex_lock,
	(pfunc)_task_mutex_unlock,
    /* FIFO functions */
	(pfunc)_task_fifo_put,
	(pfunc)_task_fifo_get,
	(pfunc)_task_fifo_ioctl,
    /* memory map functions */
	(pfunc)task_mem_map_used_get,
	(pfunc)_task_mem_map_alloc,
	(pfunc)_task_mem_map_free,
#ifdef TEST_max
    /* task device interrupt functions */
	(pfunc)task_irq_alloc,
	(pfunc)_task_irq_test,
	(pfunc)task_irq_ack,
	(pfunc)task_irq_free,
    /* semaphore functions */
	(pfunc)isr_sem_give,
	(pfunc)task_sem_give,
	(pfunc)task_sem_group_give,
	(pfunc)task_sem_count_get,
	(pfunc)task_sem_reset,
	(pfunc)task_sem_group_reset,
	(pfunc)_task_sem_take,
	(pfunc)_task_sem_group_take,
    /* pipe functions */
	(pfunc)task_pipe_put,
	(pfunc)task_pipe_put_wait,
	(pfunc)task_pipe_put_wait_timeout,
	(pfunc)task_pipe_get,
	(pfunc)task_pipe_get_wait,
	(pfunc)task_pipe_get_wait_timeout,
	(pfunc)_task_pipe_put_async,
	(pfunc)_task_pipe_put,
	(pfunc)_task_pipe_get,
    /* mailbox functions */
	(pfunc)_task_mbox_put,
	(pfunc)_task_mbox_get,
	(pfunc)_task_mbox_put_async,
	(pfunc)_task_mbox_data_get,
	(pfunc)_task_mbox_data_get_async_block,
    /* memory pool functions */
	(pfunc)_task_mem_pool_alloc,
	(pfunc)task_mem_pool_free,
	(pfunc)task_mem_pool_defragment,
    /* task functions */
	(pfunc)_task_ioctl,
	(pfunc)_task_group_ioctl,
	(pfunc)task_abort_handler_set,
	(pfunc)task_entry_set,
	(pfunc)task_priority_set,
	(pfunc)task_sleep,
	(pfunc)task_yield,
#endif /* TEST_max */
	};

/*******************************************************************************
 *
 * dummyIsr - dummy ISR
 *
 * RETURNS: N/A
 */

void dummyIsr (void *unused)
	{
	ARG_UNUSED (unused);
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

static void isrDummyIntStub (void *unused)
	{
	ARG_UNUSED (unused);

	isr_dummy();

	CODE_UNREACHABLE;
	}
#endif /* TEST_reg */

/*******************************************************************************
 *
 * fgTaskEntry - entry function for foreground task
 *
 * This routine prints a message, then simply increments a global counter.
 * (Gdb can be used to observe the counter as it increases.)
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 */

void fgTaskEntry (void)
	{
#ifdef TEST_max
    /* dynamically link in dummy ISR */
	irq_connect (NANO_SOFT_IRQ, IRQ_PRIORITY, dummyIsr,
		       (void *) 0, isrDummyHandlerStub);
#endif /* TEST_max */

    /* note: referencing "func_array" ensures it isn't optimized out */
#ifdef TEST_max
	printf ((char *)MESSAGE, func_array);
#else
	printk ((char *)MESSAGE, func_array);
#endif /* TEST_max */

	while (1) {
	i++;
	}
	}

#endif /* TEST_min */
