/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACKSIZE)
#define PIPE_LEN	(4 * 16)
#define BYTES_TO_WRITE	16
#define BYTES_TO_READ BYTES_TO_WRITE

static ZTEST_DMEM unsigned char __aligned(4) data[] =
"abcd1234$%^&PIPEefgh5678!/?*EPIPijkl9012[]<>PEPImnop3456{}()IPEP";
BUILD_ASSERT(sizeof(data) >= PIPE_LEN);

/**TESTPOINT: init via K_PIPE_DEFINE*/
K_PIPE_DEFINE(kpipe, PIPE_LEN, 4);
K_PIPE_DEFINE(khalfpipe, (PIPE_LEN / 2), 4);
K_PIPE_DEFINE(kpipe1, PIPE_LEN, 4);
K_PIPE_DEFINE(pipe_test_alloc, PIPE_LEN, 4);
struct k_pipe pipe;

K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack2, STACK_SIZE);
struct k_thread tdata;
struct k_thread tdata1;
struct k_thread tdata2;
K_SEM_DEFINE(end_sema, 0, 1);

static void tpipe_put(struct k_pipe *ppipe, k_timeout_t timeout)
{
	size_t to_wt, wt_byte = 0;

	for (int i = 0; i < PIPE_LEN; i += wt_byte) {
		/**TESTPOINT: pipe put*/
		to_wt = (PIPE_LEN - i) >= BYTES_TO_WRITE ?
			BYTES_TO_WRITE : (PIPE_LEN - i);
		zassert_false(k_pipe_put(ppipe, &data[i], to_wt,
					 &wt_byte, 1, timeout), NULL);
		zassert_true(wt_byte == to_wt || wt_byte == 1, NULL);
	}
}

static void tpipe_get(struct k_pipe *ppipe, k_timeout_t timeout)
{
	unsigned char rx_data[PIPE_LEN];
	size_t to_rd, rd_byte = 0;

	/*get pipe data from "pipe_put"*/
	for (int i = 0; i < PIPE_LEN; i += rd_byte) {
		/**TESTPOINT: pipe get*/
		to_rd = (PIPE_LEN - i) >= BYTES_TO_READ ?
			BYTES_TO_READ : (PIPE_LEN - i);
		zassert_false(k_pipe_get(ppipe, &rx_data[i], to_rd,
					 &rd_byte, 1, timeout), NULL);
		zassert_true(rd_byte == to_rd || rd_byte == 1, NULL);
	}
	for (int i = 0; i < PIPE_LEN; i++) {
		zassert_equal(rx_data[i], data[i], NULL);
	}
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tpipe_get((struct k_pipe *)p1, K_FOREVER);
	k_sem_give(&end_sema);

	tpipe_put((struct k_pipe *)p1, K_NO_WAIT);
	k_sem_give(&end_sema);
}

static void tpipe_thread_thread(struct k_pipe *ppipe)
{
	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_entry, ppipe, NULL, NULL,
				      K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	tpipe_put(ppipe, K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);

	k_sem_take(&end_sema, K_FOREVER);
	tpipe_get(ppipe, K_FOREVER);

	/* clear the spawned thread avoid side effect */
	k_thread_abort(tid);
}

static void tpipe_put_no_wait(struct k_pipe *ppipe)
{
	size_t to_wt, wt_byte = 0;

	for (int i = 0; i < PIPE_LEN; i += wt_byte) {
	/**TESTPOINT: pipe put*/
		to_wt = (PIPE_LEN - i) >= BYTES_TO_WRITE ?
			BYTES_TO_WRITE : (PIPE_LEN - i);
		zassert_false(k_pipe_put(ppipe, &data[i], to_wt,
					&wt_byte, 1, K_NO_WAIT), NULL);
		zassert_true(wt_byte == to_wt || wt_byte == 1, NULL);
	}
}

static void thread_handler(void *p1, void *p2, void *p3)
{
	tpipe_put_no_wait((struct k_pipe *)p1);
	k_sem_give(&end_sema);
}

/**
 * @addtogroup kernel_pipe_tests
 * @{
 */

/**
 * @brief Test pipe data passing between threads
 *
 * @ingroup kernel_pipe_tests
 *
 * @details
 * Test Objective:
 * - Verify data passing with "pipe put/get" APIs between
 * threads
 *
 * Testing techniques:
 * - function and block box testing,Interface testing,
 * Dynamic analysis and testing.
 *
 * Prerequisite Conditions:
 * - CONFIG_TEST_USERSPACE.
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Initialize a pipe, which is defined at run time.
 * -# Create a thread (A).
 * -# In A thread, check if it can get data, which is sent
 * by main thread via the pipe.
 * -# In A thread, send data to main thread via the pipe.
 * -# In main thread, send data to A thread  via the pipe.
 * -# In main thread, check if it can get data, which is sent
 * by A thread.
 * -# Do the same testing with a pipe, which is defined at compile
 * time
 *
 * Expected Test Result:
 * - Data can be sent/received between threads.
 *
 * Pass/Fail Criteria:
 * - Successful if check points in test procedure are all passed, otherwise failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @see k_pipe_init(), k_pipe_put(), #K_PIPE_DEFINE(x)
 */

void test_pipe_thread2thread(void)
{
	/**TESTPOINT: test k_pipe_init pipe*/

	k_pipe_init(&pipe, data, PIPE_LEN);
	tpipe_thread_thread(&pipe);

	/**TESTPOINT: test K_PIPE_DEFINE pipe*/
	tpipe_thread_thread(&kpipe);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test data passing using pipes between user threads
 * @see k_pipe_init(), k_pipe_put(), #K_PIPE_DEFINE(x)
 */
void test_pipe_user_thread2thread(void)
{
	/**TESTPOINT: test k_object_alloc pipe*/
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);

	zassert_true(p != NULL, NULL);

	/**TESTPOINT: test k_pipe_alloc_init*/
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN), NULL);
	tpipe_thread_thread(p);

}
#endif

static void tThread_half_pipe_put(void *p1, void *p2, void *p3)
{
	tpipe_put((struct k_pipe *)p1, K_FOREVER);
}

/**
 * @brief Test get/put with smaller pipe buffer
 * @see k_pipe_put(), k_pipe_get()
 */
void test_half_pipe_get_put(void)
{
	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_half_pipe_put, &khalfpipe,
				      NULL, NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	tpipe_get(&khalfpipe, K_FOREVER);

	/* clear the spawned thread avoid side effect */
	k_thread_abort(tid);
}

/**
 * @brief Test pending reader in pipe
 * @see k_pipe_put(), k_pipe_get()
 */
void test_pipe_reader_wait(void)
{
	/**TESTPOINT: test k_pipe_block_put with semaphore*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
					thread_handler, &kpipe1, NULL, NULL,
					K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	tpipe_get(&kpipe1, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);
}

/**
 * @}
 */
