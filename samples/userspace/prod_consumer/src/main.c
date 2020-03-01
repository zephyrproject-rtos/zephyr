/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <sys/printk.h>
#include <app_memory/app_memdomain.h>
#include <sys/libc-hooks.h>
#include <sys/mempool.h>
#include <logging/log.h>

#include "main.h"
#include "sample_driver.h"
#include "app_a.h"
#include "app_b.h"

#define APP_A_STACKSIZE	2048

LOG_MODULE_REGISTER(app_main);

/* Define the shared partition, which will contain a memory region that
 * will be accessible by both applications A and B.
 */
K_APPMEM_PARTITION_DEFINE(shared_partition);

/* Define a memory pool to place in the shared area.
 *
 * SYS_MEM_POOL_DEFINE() is special in that we don't use K_APP_DMEM()
 * to route it to the shared partition, instead it takes a section parameter.
 */
#define BLK_SIZE (SAMPLE_DRIVER_MSG_SIZE + \
		  WB_UP(sizeof(struct sys_mem_pool_block)))
SYS_MEM_POOL_DEFINE(shared_pool, NULL, BLK_SIZE, BLK_SIZE, 8, 8,
		    K_APP_DMEM_SECTION(shared_partition));

/* queues for exchanging data between App A and App B */
K_QUEUE_DEFINE(shared_queue_incoming);
K_QUEUE_DEFINE(shared_queue_outgoing);

/* Define a thread for the root of application A.
 * We don't do this for Application B, instead we re-use the main thread
 * for it.
 */
struct k_thread app_a_thread;
K_THREAD_STACK_DEFINE(app_a_stack, APP_A_STACKSIZE);

void main(void)
{
	LOG_INF("APP A partition: %p %zu", (void *)app_a_partition.start,
		(size_t)app_a_partition.size);
	LOG_INF("Shared partition: %p %zu", (void *)shared_partition.start,
		(size_t)shared_partition.size);
#ifdef Z_LIBC_PARTITION_EXISTS
	LOG_INF("libc partition: %p %zu", (void *)z_libc_partition.start,
		(size_t)z_libc_partition.size);
#endif
	sys_mem_pool_init(&shared_pool);

	/* Spawn supervisor entry for application A */
	k_thread_create(&app_a_thread, app_a_stack, APP_A_STACKSIZE,
			app_a_entry, NULL, NULL, NULL,
			-1, K_INHERIT_PERMS, K_NO_WAIT);

	/* Re-use main for app B supervisor mode setup */
	app_b_entry(NULL, NULL, NULL);
}
