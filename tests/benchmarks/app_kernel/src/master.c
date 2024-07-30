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

BENCH_BMEM char msg[MAX_MSG];
BENCH_BMEM char data_bench[MESSAGE_SIZE];

BENCH_DMEM struct k_pipe *test_pipes[] = {&PIPE_NOBUFF, &PIPE_SMALLBUFF, &PIPE_BIGBUFF};
BENCH_BMEM char sline[SLINE_LEN + 1];

/*
 * Time in timer cycles necessary to read time.
 * Used for correction in time measurements.
 */
uint32_t tm_off;

static BENCH_DMEM char *test_type_str[] = {
	"|                  K E R N E L - - > K E R N E L                   |          |\n",
	"|                  K E R N E L - - >   U S E R                     |          |\n",
	"|                    U S E R   - - > K E R N E L                   |          |\n",
	"|                    U S E R   - - >   U S E R                     |          |\n"
};

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
K_MSGQ_DEFINE(DEMOQX192, 192, 500, 4);
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

/*
 * Custom syscalls
 */

/**
 * @brief Obtain a timestamp
 *
 * Architecture timestamp routines often require MMIO that is not mapped to
 * the user threads. Use a custom system call to get the timestamp.
 */
timing_t z_impl_timing_timestamp_get(void)
{
	return timing_counter_get();
}

#ifdef CONFIG_USERSPACE
static timing_t z_vrfy_timing_timestamp_get(void)
{
	return z_impl_timing_timestamp_get();
}

#include <zephyr/syscalls/timing_timestamp_get_mrsh.c>
#endif

/*
 * Main test
 */

/**
 * @brief Entry point for test thread
 */
static void test_thread_entry(void *p1, void *p2, void *p3)
{
	bool skip_mem_and_mbox = (bool)(uintptr_t)(p2);

	ARG_UNUSED(p3);

	PRINT_STRING("\n");
	PRINT_STRING(dashline);
	PRINT_STRING("|          S I M P L E   S E R V I C E    "
		     "M E A S U R E M E N T S  |  nsec    |\n");
#ifdef CONFIG_USERSPACE
	PRINT_STRING((const char *)p1);
#endif
	PRINT_STRING(dashline);

	message_queue_test();
	sema_test();
	mutex_test();

	if (!skip_mem_and_mbox) {
		memorymap_test();
		mailbox_test();
	}

	pipe_test();
}

/**
 * @brief Perform all benchmarks
 */
int main(void)
{
	int  priority;

	priority = k_thread_priority_get(k_current_get());

#ifdef CONFIG_USERSPACE
	k_mem_domain_add_partition(&k_mem_domain_default,
				   &bench_mem_partition);
#endif

	bench_test_init();

	timing_init();

	timing_start();

	/* ********* All threads are kernel threads ********** */

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			test_thread_entry,
			test_type_str[0], (void *)(uintptr_t)false, NULL,
			priority, 0, K_FOREVER);

	k_thread_create(&recv_thread, recv_stack,
			K_THREAD_STACK_SIZEOF(recv_stack),
			recvtask, (void *)(uintptr_t)false, NULL, NULL,
			5, 0, K_FOREVER);

	k_thread_start(&recv_thread);
	k_thread_start(&test_thread);

	k_thread_join(&test_thread, K_FOREVER);
	k_thread_abort(&recv_thread);

#ifdef CONFIG_USERSPACE
	/* ****** Main thread is kernel, receiver is user thread ******* */

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			test_thread_entry,
			test_type_str[1], (void *)(uintptr_t)true, NULL,
			priority, 0, K_FOREVER);

	k_thread_create(&recv_thread, recv_stack,
			K_THREAD_STACK_SIZEOF(recv_stack),
			recvtask, (void *)(uintptr_t)true, NULL, NULL,
			5, K_USER, K_FOREVER);

	k_thread_access_grant(&recv_thread, &DEMOQX1, &DEMOQX4, &DEMOQX192,
			      &MB_COMM, &CH_COMM, &SEM0, &SEM1, &SEM2, &SEM3,
			      &SEM4, &STARTRCV, &DEMO_MUTEX,
			      &PIPE_NOBUFF, &PIPE_SMALLBUFF, &PIPE_BIGBUFF);

	k_thread_start(&recv_thread);
	k_thread_start(&test_thread);

	k_thread_join(&test_thread, K_FOREVER);
	k_thread_abort(&recv_thread);

	/* ****** Main thread is user, receiver is kernel thread ******* */

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			test_thread_entry,
			test_type_str[2], (void *)(uintptr_t)true, NULL,
			priority, K_USER, K_FOREVER);

	k_thread_create(&recv_thread, recv_stack,
			K_THREAD_STACK_SIZEOF(recv_stack),
			recvtask, (void *)(uintptr_t)true, NULL, NULL,
			5, 0, K_FOREVER);

	k_thread_access_grant(&test_thread, &DEMOQX1, &DEMOQX4, &DEMOQX192,
			      &MB_COMM, &CH_COMM, &SEM0, &SEM1, &SEM2, &SEM3,
			      &SEM4, &STARTRCV, &DEMO_MUTEX,
			      &PIPE_NOBUFF, &PIPE_SMALLBUFF, &PIPE_BIGBUFF);

	k_thread_start(&recv_thread);
	k_thread_start(&test_thread);

	k_thread_join(&test_thread, K_FOREVER);
	k_thread_abort(&recv_thread);

	/* ********* All threads are user threads ********** */

	k_thread_create(&test_thread, test_stack,
			K_THREAD_STACK_SIZEOF(test_stack),
			test_thread_entry,
			test_type_str[3], (void *)(uintptr_t)true, NULL,
			priority, K_USER, K_FOREVER);

	k_thread_create(&recv_thread, recv_stack,
			K_THREAD_STACK_SIZEOF(recv_stack),
			recvtask, (void *)(uintptr_t)true, NULL, NULL,
			5, K_USER, K_FOREVER);

	k_thread_access_grant(&test_thread, &DEMOQX1, &DEMOQX4, &DEMOQX192,
			      &MB_COMM, &CH_COMM, &SEM0, &SEM1, &SEM2, &SEM3,
			      &SEM4, &STARTRCV, &DEMO_MUTEX,
			      &PIPE_NOBUFF, &PIPE_SMALLBUFF, &PIPE_BIGBUFF);
	k_thread_access_grant(&recv_thread, &DEMOQX1, &DEMOQX4, &DEMOQX192,
			      &MB_COMM, &CH_COMM, &SEM0, &SEM1, &SEM2, &SEM3,
			      &SEM4, &STARTRCV, &DEMO_MUTEX,
			      &PIPE_NOBUFF, &PIPE_SMALLBUFF, &PIPE_BIGBUFF);

	k_thread_start(&recv_thread);
	k_thread_start(&test_thread);

	k_thread_join(&test_thread, K_FOREVER);
	k_thread_abort(&recv_thread);
#endif /* CONFIG_USERSPACE */

	timing_stop();

	PRINT_STRING("|         END OF TESTS                     "
		     "                                   |\n");
	PRINT_STRING(dashline);
	PRINT_STRING("PROJECT EXECUTION SUCCESSFUL\n");
	TC_PRINT_RUNID;

	return 0;
}
