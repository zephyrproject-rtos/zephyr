/*
 * Copyright (c) 2017, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <misc/util.h>
#include <misc/__assert.h>

#include <inc/hw_types.h>
#include <inc/hw_ints.h>
#include <driverlib/rom_map.h>
#include <driverlib/interrupt.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <kernel/zephyr/dpl/dpl.h>

#define	 SPAWN_TASK_STACKSIZE  2048
/*
 * Priority must be higher than any thread priority in the system which
 * might use the SimpleLink host driver, which involves the spawn_task().
 * Since SimpleLink APIs may be called from any thread, including
 * cooperative threads, and the _main kernel thread, we must set this
 * as highest prioirty.
 */
#define	 SPAWN_TASK_PRIORITY   K_HIGHEST_THREAD_PRIO

/* Spawn message queue size: Could be 1, but 3 is used by other DPL ports */
#define SPAWN_QUEUE_SIZE  ( 3 )

/* Stack, for the simplelink spawn task: */
static K_THREAD_STACK_DEFINE(spawn_task_stack, SPAWN_TASK_STACKSIZE);
static struct k_thread spawn_task_data;

static void spawn_task(void *unused1, void *unused2, void *unused3);

/*
 * MessageQ to send message from an ISR or other task to the SimpleLink
 * "Spawn" task:
 */
K_MSGQ_DEFINE(spawn_msgq, sizeof(tSimpleLinkSpawnMsg), SPAWN_QUEUE_SIZE,\
	      MEM_ALIGN);

/*
 * SimpleLink does not have an init hook, so we export this function to
 * be called early during system initialization.
 */
static int dpl_zephyr_init(struct device *port)
{
	(void)k_thread_create(&spawn_task_data, spawn_task_stack,
			SPAWN_TASK_STACKSIZE, spawn_task,
			NULL, NULL, NULL,
			SPAWN_TASK_PRIORITY, 0, K_NO_WAIT);
	return 0;
}
SYS_INIT(dpl_zephyr_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

/* SimpleLink driver code can call this from ISR or task context: */
_SlReturnVal_t os_Spawn(P_OS_SPAWN_ENTRY pEntry, void *pValue,
			unsigned long flags)
{
	tSimpleLinkSpawnMsg slMsg;
	_SlReturnVal_t retval;

	slMsg.pEntry = pEntry;
	slMsg.pValue = pValue;

	if (0 == k_msgq_put(&spawn_msgq, &slMsg, K_NO_WAIT)) {
		retval = OS_OK;
	}
	else {
		retval = -1;
		__ASSERT(retval == OS_OK,
			 "os_Spawn: Failed to k_msgq_put failed\r\n");

	}

	return retval;
}

void spawn_task(void *unused1, void *unused2, void *unused3)
{
	tSimpleLinkSpawnMsg slMsg;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	while (1) {
		k_msgq_get(&spawn_msgq, &slMsg, K_FOREVER);
		slMsg.pEntry(slMsg.pValue);
	}
}

#if CONFIG_ERRNO && !defined(SL_INC_INTERNAL_ERRNO)
/*
 * Called by the SimpleLink host driver to set POSIX error codes
 * for the host OS.
 */
int dpl_set_errno(int err)
{
	/* Ensure (POSIX) errno is positive.
	 * __errno() is a Zephyr function returning a pointer to the
	 * current thread's errno variable.
	 */
	*__errno() = (err < 0? -err : err);
	return -1;
}
#endif
