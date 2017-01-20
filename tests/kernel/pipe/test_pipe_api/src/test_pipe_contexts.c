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
#include <irq_offload.h>

#define STACK_SIZE 512
#define PIPE_LEN 8
#define BYTES_TO_WRITE (PIPE_LEN>>1)
#define BYTES_TO_READ BYTES_TO_WRITE

static unsigned char __aligned(4) data[] = "abcd1234";
/**TESTPOINT: init via K_PIPE_DEFINE*/
K_PIPE_DEFINE(kpipe, PIPE_LEN, 4);
static struct k_pipe pipe;

static char __noinit __stack tstack[STACK_SIZE];
static struct k_sem end_sema;

static void tpipe_put(struct k_pipe *ppipe)
{
	size_t to_wt, wt_byte = 0;

	for (int i = 0; i < PIPE_LEN; i += wt_byte) {
		/**TESTPOINT: pipe put*/
		to_wt = (PIPE_LEN - i) >= BYTES_TO_WRITE ?
			BYTES_TO_WRITE : (PIPE_LEN - i);
		assert_false(k_pipe_put(ppipe, &data[i], to_wt,
				&wt_byte, 1, K_NO_WAIT), NULL);
		assert_true(wt_byte == to_wt || wt_byte == 1, NULL);
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
		assert_false(k_pipe_get(ppipe, &rx_data[i], to_rd,
				&rd_byte, 1, K_NO_WAIT), NULL);
		assert_true(rd_byte == to_rd || rd_byte == 1, NULL);
	}
	for (int i = 0; i < PIPE_LEN; i++) {
		assert_equal(rx_data[i], data[i], NULL);
	}
}

/*entry of contexts*/
static void tIsr_entry_put(void *p)
{
	tpipe_put((struct k_pipe *)p);
}

static void tIsr_entry_get(void *p)
{
	tpipe_get((struct k_pipe *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	tpipe_get((struct k_pipe *)p1);
	k_sem_give(&end_sema);

	tpipe_put((struct k_pipe *)p1);
	k_sem_give(&end_sema);
}

static void tpipe_thread_thread(struct k_pipe *ppipe)
{
	k_sem_init(&end_sema, 0, 1);

	/**TESTPOINT: thread-thread data passing via pipe*/
	k_tid_t tid = k_thread_spawn(tstack, STACK_SIZE,
		tThread_entry, ppipe, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, 0);
	tpipe_put(ppipe);
	k_sem_take(&end_sema, K_FOREVER);

	k_sem_take(&end_sema, K_FOREVER);
	tpipe_get(ppipe);

	/* clear the spawned thread avoid side effect */
	k_thread_abort(tid);
}

static void tpipe_thread_isr(struct k_pipe *ppipe)
{
	k_sem_init(&end_sema, 0, 1);

	/**TESTPOINT: thread-isr data passing via pipe*/
	irq_offload(tIsr_entry_put, ppipe);
	tpipe_get(ppipe);

	tpipe_put(ppipe);
	irq_offload(tIsr_entry_get, ppipe);
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

void test_pipe_thread2isr(void)
{
	/**TESTPOINT: test k_pipe_init pipe*/
	k_pipe_init(&pipe, data, PIPE_LEN);
	tpipe_thread_isr(&pipe);

	/**TESTPOINT: test K_PIPE_DEFINE pipe*/
	tpipe_thread_isr(&kpipe);
}
