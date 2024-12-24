/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PIPE_LEN	(4 * 16)
#define BYTES_TO_WRITE	16
#define BYTES_TO_READ BYTES_TO_WRITE

K_HEAP_DEFINE(mpool, PIPE_LEN * 1);

static ZTEST_DMEM unsigned char __aligned(4) data[] =
"abcd1234$%^&PIPEefgh5678!/?*EPIPijkl9012[]<>PEPImnop3456{}()IPEP";
BUILD_ASSERT(sizeof(data) >= PIPE_LEN);

/**TESTPOINT: init via K_PIPE_DEFINE*/
K_PIPE_DEFINE(kpipe, PIPE_LEN, 4);
K_PIPE_DEFINE(khalfpipe, (PIPE_LEN / 2), 4);
K_PIPE_DEFINE(kpipe1, PIPE_LEN, 4);
K_PIPE_DEFINE(pipe_test_alloc, PIPE_LEN, 4);
K_PIPE_DEFINE(ksmallpipe, 10, 2);
struct k_pipe pipe, pipe1;

K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack2, STACK_SIZE);
struct k_thread tdata;
struct k_thread tdata1;
struct k_thread tdata2;
K_SEM_DEFINE(end_sema, 0, 1);

#ifdef CONFIG_64BIT
#define SZ 256
#else
#define SZ 128
#endif
K_HEAP_DEFINE(test_pool, SZ * 4);

struct mem_block {
	void *data;
};

static void tpipe_put(struct k_pipe *ppipe, k_timeout_t timeout)
{
	size_t to_wt, wt_byte = 0;

	for (int i = 0; i < PIPE_LEN; i += wt_byte) {
		/**TESTPOINT: pipe put*/
		to_wt = (PIPE_LEN - i) >= BYTES_TO_WRITE ?
			BYTES_TO_WRITE : (PIPE_LEN - i);
		zassert_false(k_pipe_put(ppipe, &data[i], to_wt,
					 &wt_byte, 1, timeout), NULL);
		zassert_true(wt_byte == to_wt || wt_byte == 1);
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
		zassert_true(rd_byte == to_rd || rd_byte == 1);
	}
	for (int i = 0; i < PIPE_LEN; i++) {
		zassert_equal(rx_data[i], data[i]);
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

static void tpipe_kthread_to_kthread(struct k_pipe *ppipe)
{
	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_entry, ppipe, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

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
		zassert_true(wt_byte == to_wt || wt_byte == 1);
	}
}

static void tpipe_put_small_size(struct k_pipe *ppipe, k_timeout_t timeout)
{
	size_t to_wt, wt_byte = 0;

	for (int i = 0; i < PIPE_LEN; i += wt_byte) {
		/**TESTPOINT: pipe put*/
		to_wt = 15;
		zassert_false(k_pipe_put(ppipe, &data[i], to_wt, &wt_byte,
		1, timeout) != 0, NULL);

	}
}

static void tpipe_get_small_size(struct k_pipe *ppipe, k_timeout_t timeout)
{
	unsigned char rx_data[PIPE_LEN];
	size_t to_rd, rd_byte = 0;

	/*get pipe data from "pipe_put"*/
	for (int i = 0; i < PIPE_LEN; i += rd_byte) {
		/**TESTPOINT: pipe get*/
		to_rd = 15;
		zassert_false(k_pipe_get(ppipe, &rx_data[i], to_rd,
					 &rd_byte, 1, timeout), NULL);
	}
}


static void tpipe_get_large_size(struct k_pipe *ppipe, k_timeout_t timeout)
{
	unsigned char rx_data[PIPE_LEN];
	size_t to_rd, rd_byte = 0;

	/*get pipe data from "pipe_put"*/
	for (int i = 0; i < PIPE_LEN; i += rd_byte) {
		/**TESTPOINT: pipe get*/
		to_rd = (PIPE_LEN - i) >= 128 ?
			128 : (PIPE_LEN - i);
		zassert_false(k_pipe_get(ppipe, &rx_data[i], to_rd,
					 &rd_byte, 1, timeout), NULL);
	}
}


/**
 * @brief Test Initialization and buffer allocation of pipe,
 * with various parameters
 * @see k_pipe_alloc_init(), k_pipe_cleanup()
 */
ZTEST(pipe_api_1cpu, test_pipe_alloc)
{
	int ret;

	zassert_false(k_pipe_alloc_init(&pipe_test_alloc, PIPE_LEN));

	tpipe_kthread_to_kthread(&pipe_test_alloc);
	k_pipe_cleanup(&pipe_test_alloc);

	zassert_false(k_pipe_alloc_init(&pipe_test_alloc, 0));
	k_pipe_cleanup(&pipe_test_alloc);

	ret = k_pipe_alloc_init(&pipe_test_alloc, 2048);
	zassert_true(ret == -ENOMEM,
		"resource pool max block size is not smaller then requested buffer");
}

static void thread_for_get_forever(void *p1, void *p2, void *p3)
{
	tpipe_get((struct k_pipe *)p1, K_FOREVER);
}

ZTEST(pipe_api, test_pipe_cleanup)
{
	int ret;

	zassert_false(k_pipe_alloc_init(&pipe_test_alloc, PIPE_LEN));

	/**TESTPOINT: test if a dynamically allocated buffer can be freed*/
	ret = k_pipe_cleanup(&pipe_test_alloc);
	zassert_true((ret == 0) && (pipe_test_alloc.buffer == NULL),
			"Failed to free buffer with k_pipe_cleanup().");

	/**TESTPOINT: nothing to do with k_pipe_cleanup() for static buffer in pipe*/
	ret = k_pipe_cleanup(&kpipe);
	zassert_true((ret == 0) && (kpipe.buffer != NULL),
			"Static buffer should not be freed.");

	zassert_false(k_pipe_alloc_init(&pipe_test_alloc, PIPE_LEN));

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread_for_get_forever, &pipe_test_alloc, NULL,
			NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_sleep(K_MSEC(100));

	ret = k_pipe_cleanup(&pipe_test_alloc);
	zassert_true(ret == -EAGAIN, "k_pipe_cleanup() should not return with 0.");
	k_thread_abort(tid);
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

ZTEST(pipe_api_1cpu, test_pipe_thread2thread)
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
ZTEST_USER(pipe_api_1cpu, test_pipe_user_thread2thread)
{
	/**TESTPOINT: test k_object_alloc pipe*/
	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);

	zassert_true(p != NULL);

	/**TESTPOINT: test k_pipe_alloc_init*/
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN));
	tpipe_thread_thread(p);

}
#endif

/**
 * @brief Test resource pool free
 * @see k_heap_alloc()
 */
#ifdef CONFIG_USERSPACE
ZTEST(pipe_api, test_resource_pool_auto_free)
{
	/* Pool has 2 blocks, both should succeed if kernel object and pipe
	 * buffer are auto-freed when the allocating threads exit
	 */
	zassert_true(k_heap_alloc(&test_pool, 64, K_NO_WAIT) != NULL);
	zassert_true(k_heap_alloc(&test_pool, 64, K_NO_WAIT) != NULL);
}
#endif

static void tThread_half_pipe_put(void *p1, void *p2, void *p3)
{
	tpipe_put((struct k_pipe *)p1, K_FOREVER);
}

static void tThread_half_pipe_get(void *p1, void *p2, void *p3)
{
	tpipe_get((struct k_pipe *)p1, K_FOREVER);
}

/**
 * @brief Test put/get with smaller pipe buffer
 * @see k_pipe_put(), k_pipe_get()
 */
ZTEST(pipe_api, test_half_pipe_put_get)
{
	unsigned char rx_data[PIPE_LEN];
	size_t rd_byte = 0;
	int ret;

	memset(rx_data, 0, sizeof(rx_data));

	/* TESTPOINT: min_xfer > bytes_to_read */
	ret = k_pipe_put(&kpipe, &rx_data[0], 1, &rd_byte, 24, K_NO_WAIT);
	zassert_true(ret == -EINVAL);
	ret = k_pipe_put(&kpipe, &rx_data[0], 24, NULL, 1, K_NO_WAIT);
	zassert_true(ret == -EINVAL);

	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid1 = k_thread_create(&tdata1, tstack1, STACK_SIZE,
				      tThread_half_pipe_get, &khalfpipe,
				      NULL, NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_tid_t tid2 = k_thread_create(&tdata2, tstack2, STACK_SIZE,
				      tThread_half_pipe_get, &khalfpipe,
				      NULL, NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_sleep(K_MSEC(100));
	tpipe_put_small_size(&khalfpipe, K_NO_WAIT);

	/* clear the spawned thread avoid side effect */
	k_thread_abort(tid1);
	k_thread_abort(tid2);
}

ZTEST(pipe_api, test_pipe_get_put)
{
	unsigned char rx_data[PIPE_LEN];
	size_t rd_byte = 0;
	int ret;

	/* TESTPOINT: min_xfer > bytes_to_read */
	ret = k_pipe_get(&kpipe, &rx_data[0], 1, &rd_byte, 24, K_NO_WAIT);
	zassert_true(ret == -EINVAL);
	ret = k_pipe_get(&kpipe, &rx_data[0], 24, NULL, 1, K_NO_WAIT);
	zassert_true(ret == -EINVAL);

	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid1 = k_thread_create(&tdata1, tstack1, STACK_SIZE,
				      tThread_half_pipe_put, &khalfpipe,
				      NULL, NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_tid_t tid2 = k_thread_create(&tdata2, tstack2, STACK_SIZE,
				      tThread_half_pipe_put, &khalfpipe,
				      NULL, NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_sleep(K_MSEC(100));
	tpipe_get_small_size(&khalfpipe, K_NO_WAIT);

	/* clear the spawned thread avoid side effect */
	k_thread_abort(tid1);
	k_thread_abort(tid2);
}

ZTEST(pipe_api, test_pipe_get_large)
{
	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid1 = k_thread_create(&tdata1, tstack1, STACK_SIZE,
				      tThread_half_pipe_put, &khalfpipe,
				      NULL, NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_tid_t tid2 = k_thread_create(&tdata2, tstack2, STACK_SIZE,
				      tThread_half_pipe_put, &khalfpipe,
				      NULL, NULL, K_PRIO_PREEMPT(0),
				      K_INHERIT_PERMS | K_USER, K_NO_WAIT);

	k_sleep(K_MSEC(100));
	tpipe_get_large_size(&khalfpipe, K_NO_WAIT);

	/* clear the spawned thread avoid side effect */
	k_thread_abort(tid1);
	k_thread_abort(tid2);
}


/**
 * @brief Test pending reader in pipe
 * @see k_pipe_put(), k_pipe_get()
 */
ZTEST(pipe_api, test_pipe_reader_wait)
{
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
