/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_kernel_msgq
 * @{
 * @defgroup t_msgq_context test_msgq_context
 * @brief TestPurpose: verify zephyr msgq apis across contexts
 * @}
 */

#include "test_msgq.h"

/**TESTPOINT: init via K_MSGQ_DEFINE*/
K_MSGQ_DEFINE(kmsgq, MSG_SIZE, MSGQ_LEN, 4);

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static u32_t data[MSGQ_LEN] = { MSG0, MSG1 };
static struct k_sem end_sema;

static void put_msgq(struct k_msgq *pmsgq)
{
	int ret;

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(pmsgq, (void *)&data[i], K_NO_WAIT);
		zassert_false(ret, NULL);
		/**TESTPOINT: msgq free get*/
		zassert_equal(k_msgq_num_free_get(pmsgq), MSGQ_LEN - 1 - i, NULL);
		/**TESTPOINT: msgq used get*/
		zassert_equal(k_msgq_num_used_get(pmsgq), i + 1, NULL);
	}
}

static void get_msgq(struct k_msgq *pmsgq)
{
	u32_t rx_data;
	int ret;

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_get(pmsgq, &rx_data, K_FOREVER);
		zassert_false(ret, NULL);
		zassert_equal(rx_data, data[i], NULL);
		/**TESTPOINT: msgq free get*/
		zassert_equal(k_msgq_num_free_get(pmsgq), i + 1, NULL);
		/**TESTPOINT: msgq used get*/
		zassert_equal(k_msgq_num_used_get(pmsgq), MSGQ_LEN - 1 - i, NULL);
	}
}

static void purge_msgq(struct k_msgq *pmsgq)
{
	k_msgq_purge(pmsgq);
	zassert_equal(k_msgq_num_free_get(pmsgq), MSGQ_LEN, NULL);
	zassert_equal(k_msgq_num_used_get(pmsgq), 0, NULL);
}

static void tIsr_entry(void *p)
{
	put_msgq((struct k_msgq *)p);
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	get_msgq((struct k_msgq *)p1);
	k_sem_give(&end_sema);
}

static void msgq_thread(struct k_msgq *pmsgq)
{
	k_sem_init(&end_sema, 0, 1);
	/**TESTPOINT: thread-thread data passing via message queue*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tThread_entry, pmsgq, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, 0);
	put_msgq(pmsgq);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/**TESTPOINT: msgq purge*/
	purge_msgq(pmsgq);
}

static void msgq_isr(struct k_msgq *pmsgq)
{
	/**TESTPOINT: thread-isr data passing via message queue*/
	irq_offload(tIsr_entry, pmsgq);
	get_msgq(pmsgq);

	/**TESTPOINT: msgq purge*/
	purge_msgq(pmsgq);
}


/*test cases*/
void test_msgq_thread(void)
{
	struct k_msgq msgq;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	msgq_thread(&msgq);
	msgq_thread(&kmsgq);
}

void test_msgq_isr(void)
{
	struct k_msgq msgq;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	msgq_isr(&msgq);
	msgq_isr(&kmsgq);
}
