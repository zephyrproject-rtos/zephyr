/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define STACK_SIZE	(1024 + CONFIG_TEST_EXTRA_STACKSIZE)
#define PIPE_LEN	16
#define BYTES_TO_WRITE	4
#define BYTES_TO_READ BYTES_TO_WRITE
K_MEM_POOL_DEFINE(mpool, BYTES_TO_WRITE, PIPE_LEN, 1, BYTES_TO_WRITE);

static ZTEST_DMEM unsigned char __aligned(4) data[] = "abcd1234$%^&PIPE";
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

/* By design, only two blocks. We should never need more than that, one
 * to allocate the pipe object, one for its buffer. Both should be auto-
 * released when the thread exits
 */
K_MEM_POOL_DEFINE(test_pool, 128, 128, 4, 4);

static void tpipe_put(struct k_pipe *ppipe, int timeout)
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

static void tpipe_block_put(struct k_pipe *ppipe, struct k_sem *sema,
			    int timeout)
{
	struct k_mem_block block;

	for (int i = 0; i < PIPE_LEN; i += BYTES_TO_WRITE) {
		/**TESTPOINT: pipe block put*/
		zassert_equal(k_mem_pool_alloc(&mpool, &block, BYTES_TO_WRITE,
					       timeout), 0, NULL);
		memcpy(block.data, &data[i], BYTES_TO_WRITE);
		k_pipe_block_put(ppipe, &block, BYTES_TO_WRITE, sema);
		if (sema) {
			k_sem_take(sema, K_FOREVER);
		}
	}
}

static void tpipe_get(struct k_pipe *ppipe, int timeout)
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

static void tThread_block_put(void *p1, void *p2, void *p3)
{
	tpipe_block_put((struct k_pipe *)p1, (struct k_sem *)p2, K_NO_WAIT);
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
		zassert_true(wt_byte == to_wt || wt_byte == 1, NULL);
	}
}

static void thread_handler(void *p1, void *p2, void *p3)
{
	tpipe_put_no_wait((struct k_pipe *)p1);
	k_sem_give(&end_sema);
}

static void thread_for_block_put(void *p1, void *p2, void *p3)
{
	tpipe_block_put((struct k_pipe *)p1, (struct k_sem *)p2, K_FOREVER);
}

/**
 * @addtogroup kernel_pipe_tests
 * @{
 */

/**
 * @brief Test pipe data passing between threads
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
	/**TESTPOINT: test k_pipe_init pipe*/

	struct k_pipe *p = k_object_alloc(K_OBJ_PIPE);

	zassert_true(p != NULL, NULL);
	zassert_false(k_pipe_alloc_init(p, PIPE_LEN), NULL);
	tpipe_thread_thread(&pipe);

	/**TESTPOINT: test K_PIPE_DEFINE pipe*/
	tpipe_thread_thread(&kpipe);
}
#endif

/**
 * @brief Test pipe put of blocks
 * @see k_pipe_block_put()
 */
void test_pipe_block_put(void)
{

	/**TESTPOINT: test k_pipe_block_put without semaphore*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_block_put, &kpipe, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_sleep(K_MSEC(10));
	tpipe_get(&kpipe, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);

	k_thread_abort(tid);
}

/**
 * @brief Test pipe block put with semaphore
 * @see k_pipe_block_put()
 */
void test_pipe_block_put_sema(void)
{
	struct k_sem sync_sema;

	k_sem_init(&sync_sema, 0, 1);
	/**TESTPOINT: test k_pipe_block_put with semaphore*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_block_put, &pipe, &sync_sema,
				      NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_sleep(K_MSEC(10));
	tpipe_get(&pipe, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);

	k_thread_abort(tid);
}

/**
 * @brief Test pipe get and put
 * @see k_pipe_put(), k_pipe_get()
 */
void test_pipe_get_put(void)
{
	/**TESTPOINT: test API sequence: [get, put]*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_block_put, &kpipe, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	/*get will be executed previous to put*/
	tpipe_get(&kpipe, K_FOREVER);
	k_sem_take(&end_sema, K_FOREVER);

	k_thread_abort(tid);
}
/**
 * @brief Test resource pool free
 * @see k_mem_pool_malloc()
 */
#ifdef CONFIG_USERSPACE
void test_resource_pool_auto_free(void)
{
	/* Pool has 2 blocks, both should succeed if kernel object and pipe
	 * buffer are auto-freed when the allocating threads exit
	 */
	zassert_true(k_mem_pool_malloc(&test_pool, 64) != NULL, NULL);
	zassert_true(k_mem_pool_malloc(&test_pool, 64) != NULL, NULL);
}
#endif

static void tThread_half_pipe_put(void *p1, void *p2, void *p3)
{
	tpipe_put((struct k_pipe *)p1, K_FOREVER);
}

static void tThread_half_pipe_block_put(void *p1, void *p2, void *p3)
{
	tpipe_block_put((struct k_pipe *)p1, (struct k_sem *)p2, K_FOREVER);
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
 * @brief Test pipe block put with semaphore and smaller pipe buffer
 * @see k_pipe_block_put()
 */
void test_half_pipe_block_put_sema(void)
{
	struct k_sem sync_sema;

	k_sem_init(&sync_sema, 0, 1);

	/**TESTPOINT: test k_pipe_block_put with semaphore*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_half_pipe_block_put,
				      &khalfpipe, &sync_sema, NULL,
				      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_sleep(K_MSEC(10));
	tpipe_get(&khalfpipe, K_FOREVER);

	k_thread_abort(tid);
}

/**
 * @brief Test Initialization and buffer allocation of pipe,
 * with various parameters
 * @see k_pipe_alloc_init(), k_pipe_cleanup()
 */
void test_pipe_alloc(void)
{
	int ret;

	zassert_false(k_pipe_alloc_init(&pipe_test_alloc, PIPE_LEN), NULL);

	tpipe_kthread_to_kthread(&pipe_test_alloc);
	k_pipe_cleanup(&pipe_test_alloc);

	zassert_false(k_pipe_alloc_init(&pipe_test_alloc, 0), NULL);
	k_pipe_cleanup(&pipe_test_alloc);

	ret = k_pipe_alloc_init(&pipe_test_alloc, PIPE_LEN * 8);
	zassert_true(ret == -ENOMEM,
		"resource pool is smaller then requested buffer");
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
 * @brief Test pending writer in pipe
 * @see k_pipe_block_put(), k_pipe_get()
 */
void test_pipe_block_writer_wait(void)
{
	struct k_sem s_sema;
	struct k_sem s_sema1;

	const int main_low_prio = 10;

	k_sem_init(&s_sema, 0, 1);
	k_sem_init(&s_sema1, 0, 1);

	int old_prio = k_thread_priority_get(k_current_get());

	k_thread_priority_set(k_current_get(), main_low_prio);
	/**TESTPOINT: test k_pipe_block_put with semaphore*/

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
					thread_for_block_put, &kpipe1, &s_sema,
					NULL, K_PRIO_PREEMPT(main_low_prio - 1),
					0, K_NO_WAIT);

	k_tid_t tid1 = k_thread_create(&tdata1, tstack1, STACK_SIZE,
					thread_for_block_put, &kpipe1, &s_sema1,
					NULL, K_PRIO_PREEMPT(main_low_prio - 1),
					0, K_NO_WAIT);

	tpipe_get(&kpipe1, K_FOREVER);
	k_thread_priority_set(k_current_get(), old_prio);
	k_thread_abort(tid);
	k_thread_abort(tid1);
}

/**
 * @}
 */
