/* microkernel_footprint.c - microkernel footprint */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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

#ifdef TEST_min

/* INTENTIONALLY LEFT EMPTY (I.E. NO APPLICATION TASKS, FIBERS, OR ISRS) */

#else

#include <misc/printk.h>
#include <stdio.h>

#define IRQ_LINE          10  /* just some random value w/o driver conflicts */
#define IRQ_PRIORITY      3
#define TEST_SOFT_INT	  64

#ifdef TEST_max
#define MESSAGE "Running maximal microkernel configuration %p\n"
#else
#define MESSAGE "Running regular microkernel configuration %p\n"
#endif /* TEST_max */

typedef void* (*pfunc) (void*);

/* variables */

volatile int i = 0;		/* counter used by foreground task */

/* pointer array ensures specified functions are linked into the image */
static pfunc func_array[] = {
	/* event functions */
	(pfunc)task_event_send,
	(pfunc)task_event_recv,
	/* mutex functions */
	(pfunc)task_mutex_lock,
	(pfunc)_task_mutex_unlock,
	/* FIFO functions */
	(pfunc)task_fifo_put,
	(pfunc)task_fifo_get,
	(pfunc)_task_fifo_ioctl,
	/* memory map functions */
	(pfunc)task_mem_map_used_get,
	(pfunc)task_mem_map_alloc,
	(pfunc)_task_mem_map_free,
#ifdef TEST_max
	/* semaphore functions */
	(pfunc)isr_sem_give,
	(pfunc)task_sem_give,
	(pfunc)task_sem_group_give,
	(pfunc)task_sem_count_get,
	(pfunc)task_sem_reset,
	(pfunc)task_sem_group_reset,
	(pfunc)task_sem_take,
	(pfunc)task_sem_group_take,
	/* pipe functions */
	(pfunc)task_pipe_put,
	(pfunc)task_pipe_get,
	(pfunc)_task_pipe_block_put,
	/* mailbox functions */
	(pfunc)task_mbox_put,
	(pfunc)task_mbox_get,
	(pfunc)_task_mbox_block_put,
	(pfunc)_task_mbox_data_get,
	(pfunc)task_mbox_data_block_get,
	/* memory pool functions */
	(pfunc)task_mem_pool_alloc,
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

/**
 *
 * @brief Entry function for foreground task
 *
 * This routine prints a message, then simply increments a global counter.
 * (Gdb can be used to observe the counter as it increases.)
 *
 * @return N/A
 */
void fgTaskEntry(void)
{
#ifdef TEST_reg
	IRQ_CONNECT(IRQ_LINE, IRQ_PRIORITY, dummyIsr, NULL, 0);
#endif
	/* note: referencing "func_array" ensures it isn't optimized out */
#ifdef TEST_max
	printf((char *)MESSAGE, func_array);
#else
	printk((char *)MESSAGE, func_array);
#endif /* TEST_max */

	while (1) {
		i++;
	}
}

#endif /* TEST_min */
