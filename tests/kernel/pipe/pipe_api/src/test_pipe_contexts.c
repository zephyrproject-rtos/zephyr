/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_pipe_api
 * @{
 * @defgroup t_pipe_api_basic test_pipe_api_basic
 * @brief TestPurpose: verify zephyr pipe apis under different context
 * - API coverage
 *   -# k_pipe_init K_PIPE_DEFINE
 *   -# k_pipe_put
 *   -# k_pipe_get
 * @}
 */

#include <ztest.h>

#define STACK_SIZE 1024
#define PIPE_LEN 16
#define BYTES_TO_WRITE 4
#define BYTES_TO_READ BYTES_TO_WRITE
K_MEM_POOL_DEFINE(mpool, BYTES_TO_WRITE, PIPE_LEN, 1, BYTES_TO_WRITE);

static unsigned char __aligned(4) data[] = "abcd1234$%^&PIPE";
/**TESTPOINT: init via K_PIPE_DEFINE*/
K_PIPE_DEFINE(kpipe, PIPE_LEN, 4);
static struct k_pipe pipe;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_sem end_sema;

static void tpipe_put(struct k_pipe *ppipe)
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

static void tpipe_block_put(struct k_pipe *ppipe, struct k_sem *sema)
{
	struct k_mem_block block;

	for (int i = 0; i < PIPE_LEN; i += BYTES_TO_WRITE) {
		/**TESTPOINT: pipe block put*/
		zassert_equal(k_mem_pool_alloc(&mpool, &block, BYTES_TO_WRITE,
					       K_NO_WAIT), 0, NULL);
		memcpy(block.data, &data[i], BYTES_TO_WRITE);
		k_pipe_block_put(ppipe, &block, BYTES_TO_WRITE, sema);
		if (sema) {
			k_sem_take(sema, K_FOREVER);
		}
		k_mem_pool_free(&block);
	}
}

static void tpipe_get(struct k_pipe *ppipe)
{
	unsigned char rx_data[PIPE_LEN];
	size_t to_rd, rd_byte = 0;

	/*get pipe data from "pipe_put"*/
	for (int i = 0; i < PIPE_LEN; i += rd_byte) {
		/**TESTPOINT: pipe get*/
		to_rd = (PIPE_LEN - i) >= BYTES_TO_READ ?
			BYTES_TO_READ : (PIPE_LEN - i);
		zassert_false(k_pipe_get(ppipe, &rx_data[i], to_rd,
					 &rd_byte, 1, K_FOREVER), NULL);
		zassert_true(rd_byte == to_rd || rd_byte == 1, NULL);
	}
	for (int i = 0; i < PIPE_LEN; i++) {
		zassert_equal(rx_data[i], data[i], NULL);
	}
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tpipe_get((struct k_pipe *)p1);
	k_sem_give(&end_sema);

	tpipe_put((struct k_pipe *)p1);
	k_sem_give(&end_sema);
}

static void tThread_block_put(void *p1, void *p2, void *p3)
{
	tpipe_block_put((struct k_pipe *)p1, (struct k_sem *)p2);
	k_sem_give(&end_sema);
}

static void tpipe_thread_thread(struct k_pipe *ppipe)
{
	k_sem_init(&end_sema, 0, 1);

	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_entry, ppipe, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, 0);
	tpipe_put(ppipe);
	k_sem_take(&end_sema, K_FOREVER);

	k_sem_take(&end_sema, K_FOREVER);
	tpipe_get(ppipe);

	/* clear the spawned thread avoid side effect */
	k_thread_abort(tid);
}

/*test cases*/
void test_pipe_thread2thread(void)
{
	/**TESTPOINT: test k_pipe_init pipe*/
	k_pipe_init(&pipe, data, PIPE_LEN);
	tpipe_thread_thread(&pipe);

	/**TESTPOINT: test K_PIPE_DEFINE pipe*/
	tpipe_thread_thread(&kpipe);
}

void test_pipe_block_put(void)
{

	/**TESTPOINT: test k_pipe_block_put without semaphore*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_block_put, &kpipe, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, 0);

	k_sleep(10);
	tpipe_get(&kpipe);
	k_sem_take(&end_sema, K_FOREVER);

	k_thread_abort(tid);
}

void test_pipe_block_put_sema(void)
{
	struct k_sem sync_sema;

	k_sem_init(&sync_sema, 0, 1);
	/**TESTPOINT: test k_pipe_block_put with semaphore*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_block_put, &pipe, &sync_sema, NULL,
				      K_PRIO_PREEMPT(0), 0, 0);
	k_sleep(10);
	tpipe_get(&pipe);
	k_sem_take(&end_sema, K_FOREVER);

	k_thread_abort(tid);
}

void test_pipe_get_put(void)
{
	/**TESTPOINT: test API sequence: [get, put]*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_block_put, &kpipe, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, 0);

	/*get will be executed previor to put*/
	tpipe_get(&kpipe);
	k_sem_take(&end_sema, K_FOREVER);

	k_thread_abort(tid);
}

