/* master.c */

/*
 * Copyright (c) 1997-2010,2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * File Naming information.
 * ------------------------
 *	Files that end with:
 *		_B : Is a file that contains a benchmark function
 *		_R : Is a file that contains the receiver task
 *			 of a benchmark function
 */
#include <zephyr/tc_util.h>
#include "master.h"

#define RECV_STACK_SIZE  (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define TEST_STACK_SIZE  (1024 + CONFIG_TEST_EXTRA_STACK_SIZE)

char msg[MAX_MSG];
char data_bench[MESSAGE_SIZE];

struct k_pipe *test_pipes[] = {&PIPE_NOBUFF, &PIPE_SMALLBUFF, &PIPE_BIGBUFF};
char sline[SLINE_LEN + 1];

/*
 * Time in timer cycles necessary to read time.
 * Used for correction in time measurements.
 */
uint32_t tm_off;

/********************************************************************/
/* static allocation  */

static struct k_thread test_thread;
static struct k_thread recv_thread;
K_THREAD_STACK_DEFINE(test_stack, TEST_STACK_SIZE);
K_THREAD_STACK_DEFINE(recv_stack, RECV_STACK_SIZE);

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(bench_mem_partition);
#endif

K_MSGQ_DEFINE(DEMOQX1, 1, 500, 4);
K_MSGQ_DEFINE(DEMOQX4, 4, 500, 4);
K_MSGQ_DEFINE(MB_COMM, 12, 1, 4);
K_MSGQ_DEFINE(CH_COMM, 12, 1, 4);

K_MEM_SLAB_DEFINE(MAP1, 16, 2, 4);

K_SEM_DEFINE(SEM0, 0, 1);
K_SEM_DEFINE(SEM1, 0, 1);
K_SEM_DEFINE(SEM2, 0, 1);
K_SEM_DEFINE(SEM3, 0, 1);
K_SEM_DEFINE(SEM4, 0, 1);
K_SEM_DEFINE(STARTRCV, 0, 1);

K_MBOX_DEFINE(MAILB1);

K_MUTEX_DEFINE(DEMO_MUTEX);

K_PIPE_DEFINE(PIPE_NOBUFF, 0, 4);
K_PIPE_DEFINE(PIPE_SMALLBUFF, 256, 4);
K_PIPE_DEFINE(PIPE_BIGBUFF, 4096, 4);

/**
 * @brief Entry point for test thread
 */
static void test_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	PRINT_STRING("\n");
	PRINT_STRING(dashline);
	PRINT_STRING("|          S I M P L E   S E R V I C E    "
		     "M E A S U R E M E N T S  |  nsec    |\n");
#ifdef CONFIG_USERSPACE
	PRINT_STRING((const char *)arg1);
#endif
	PRINT_STRING(dashline);

	message_queue_test();
	sema_test();
	mutex_test();
	memorymap_test();
	mailbox_test();
	pipe_test();
}

/**
 * @brief Perform all benchmarks
 */
int main(void)
{
	int  priority;

	priority = k_thread_priority_get(k_current_get());

	bench_test_init();

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			test_thread_entry, NULL, NULL, NULL,
			priority, 0, K_FOREVER);

	k_thread_create(&recv_thread, recv_stack,
			K_THREAD_STACK_SIZEOF(recv_stack),
			recvtask, NULL, NULL, NULL,
			5, 0, K_FOREVER);

	k_thread_start(&recv_thread);
	k_thread_start(&test_thread);

	k_thread_join(&test_thread, K_FOREVER);
	k_thread_abort(&recv_thread);

	PRINT_STRING("|         END OF TESTS                     "
		     "                                   |\n");
	PRINT_STRING(dashline);
	PRINT_STRING("PROJECT EXECUTION SUCCESSFUL\n");
	TC_PRINT_RUNID;

	return 0;
}
