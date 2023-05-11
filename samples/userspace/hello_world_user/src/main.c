/*
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <stdio.h>
#define USER_STACKSIZE	2048

struct k_thread user_thread;
K_THREAD_STACK_DEFINE(user_stack, USER_STACKSIZE);

/* Remark: userspace access to a memory region defined in the dts can be created
 * by defining a memory partition (with correct memory region alignment) as:
 * K_MEM_PARTITION_DEFINE(partition_name, mem_region_start, mem_region_size,
 *			  K_MEM_PARTITION_P_RW_U_RW);
 */
K_APPMEM_PARTITION_DEFINE(inf_part);
K_APP_DMEM(inf_part) char inf[32] = "INFO in memory partition";

static void user_function(void *p1, void *p2, void *p3)
{
	printf("Hello World from %s (%s)\n",
	       k_is_user_context() ? "UserSpace!" : "privileged mode.",
	       CONFIG_BOARD);
	printf("Memory partition data: %s\n", inf);
	__ASSERT(k_is_user_context(), "User mode execution was expected");
}


int main(void)
{
	int rc = k_mem_domain_add_partition(&k_mem_domain_default, &inf_part);

	if (rc != 0) {
		printf("Failed to add memory partition (%d)\n", rc);
	}

	k_thread_create(&user_thread, user_stack, USER_STACKSIZE,
			user_function, NULL, NULL, NULL,
			-1, K_USER, K_MSEC(0));
	return 0;
}
