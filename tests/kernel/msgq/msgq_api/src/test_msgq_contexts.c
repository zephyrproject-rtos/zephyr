/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_msgq.h"

/**TESTPOINT: init via K_MSGQ_DEFINE*/
K_MSGQ_DEFINE(kmsgq, MSG_SIZE, MSGQ_LEN, 4);
K_MSGQ_DEFINE(kmsgq_test_alloc, MSG_SIZE, MSGQ_LEN, 4);
struct k_msgq msgq;
struct k_msgq msgq1;
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack1, STACK_SIZE);
K_THREAD_STACK_DEFINE(tstack2, STACK_SIZE);
struct k_thread tdata;
struct k_thread tdata1;
struct k_thread tdata2;
static ZTEST_BMEM char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static ZTEST_DMEM char __aligned(4) tbuffer1[MSG_SIZE];
static ZTEST_DMEM uint32_t data[MSGQ_LEN] = { MSG0, MSG1 };
struct k_sem end_sema;

static void put_msgq(struct k_msgq *pmsgq)
{
	int ret;
	uint32_t read_data;

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = k_msgq_put(pmsgq, (void *)&data[i], K_NO_WAIT);
		zassert_equal(ret, 0, NULL);

		/**TESTPOINT: Check if k_msgq_peek reads msgq
		 * in FIFO manner.
		 * Everytime msg is enqueued, msg read should
		 * always be the first message
		 */
		zassert_equal(k_msgq_peek(pmsgq, &read_data), 0, NULL);
		zassert_equal(read_data, data[0], NULL);

		/**TESTPOINT: msgq free get*/
		zassert_equal(k_msgq_num_free_get(pmsgq),
				MSGQ_LEN - 1 - i, NULL);
		/**TESTPOINT: msgq used get*/
		zassert_equal(k_msgq_num_used_get(pmsgq), i + 1, NULL);
	}
}

static void get_msgq(struct k_msgq *pmsgq)
{
	uint32_t rx_data, read_data;
	int ret;

	for (int i = 0; i < MSGQ_LEN; i++) {
		zassert_equal(k_msgq_peek(pmsgq, &read_data), 0, NULL);

		ret = k_msgq_get(pmsgq, &rx_data, K_FOREVER);
		zassert_equal(ret, 0, NULL);
		zassert_equal(rx_data, data[i], NULL);

		/**TESTPOINT: Check if msg read is the msg deleted*/
		zassert_equal(read_data, rx_data, NULL);
		/**TESTPOINT: msgq free get*/
		zassert_equal(k_msgq_num_free_get(pmsgq), i + 1, NULL);
		/**TESTPOINT: msgq used get*/
		zassert_equal(k_msgq_num_used_get(pmsgq),
				MSGQ_LEN - 1 - i, NULL);
	}
}

static void purge_msgq(struct k_msgq *pmsgq)
{
	uint32_t read_data;

	k_msgq_purge(pmsgq);
	zassert_equal(k_msgq_num_free_get(pmsgq), MSGQ_LEN, NULL);
	zassert_equal(k_msgq_num_used_get(pmsgq), 0, NULL);
	zassert_equal(k_msgq_peek(pmsgq, &read_data), -ENOMSG, NULL);
}

static void tisr_entry(const void *p)
{
	put_msgq((struct k_msgq *)p);
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	get_msgq((struct k_msgq *)p1);
	k_sem_give(&end_sema);
}

static void msgq_thread(struct k_msgq *pmsgq)
{
	/**TESTPOINT: thread-thread data passing via message queue*/
	put_msgq(pmsgq);
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, pmsgq, NULL, NULL,
				      K_PRIO_PREEMPT(0),
				      K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/**TESTPOINT: msgq purge*/
	purge_msgq(pmsgq);
}

static void thread_entry_overflow(void *p1, void *p2, void *p3)
{
	int ret;

	uint32_t rx_buf[MSGQ_LEN];

	ret = k_msgq_get(p1, &rx_buf[0], K_FOREVER);

	zassert_equal(ret, 0, NULL);

	ret = k_msgq_get(p1, &rx_buf[1], K_FOREVER);

	zassert_equal(ret, 0, NULL);

	k_sem_give(&end_sema);
}

static void msgq_thread_overflow(struct k_msgq *pmsgq)
{
	int ret;

	ret = k_msgq_put(pmsgq, (void *)&data[0], K_FOREVER);

	zassert_equal(ret, 0, NULL);

	/**TESTPOINT: thread-thread data passing via message queue*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry_overflow, pmsgq, NULL, NULL,
				      K_PRIO_PREEMPT(0),
				      K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	ret = k_msgq_put(pmsgq, (void *)&data[1], K_FOREVER);

	zassert_equal(ret, 0, NULL);

	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);

	/**TESTPOINT: msgq purge*/
	k_msgq_purge(pmsgq);
}

static void msgq_isr(struct k_msgq *pmsgq)
{
	/**TESTPOINT: thread-isr data passing via message queue*/
	irq_offload(tisr_entry, (const void *)pmsgq);
	get_msgq(pmsgq);

	/**TESTPOINT: msgq purge*/
	purge_msgq(pmsgq);
}

static void thread_entry_get_data(void *p1, void *p2, void *p3)
{
	static uint32_t rx_buf[MSGQ_LEN];
	int i = 0;

	while (k_msgq_get(p1, &rx_buf[i], K_NO_WAIT) != 0) {
		++i;
	}

	k_sem_give(&end_sema);
}

static void pend_thread_entry(void *p1, void *p2, void *p3)
{
	int ret;

	ret = k_msgq_put(p1, &data[1], TIMEOUT);
	zassert_equal(ret, 0, NULL);
}

static void msgq_thread_data_passing(struct k_msgq *pmsgq)
{
	while (k_msgq_put(pmsgq, &data[0], K_NO_WAIT) != 0) {
	}

	k_tid_t tid = k_thread_create(&tdata2, tstack2, STACK_SIZE,
					pend_thread_entry, pmsgq, NULL,
					NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_tid_t tid1 = k_thread_create(&tdata1, tstack1, STACK_SIZE,
					thread_entry_get_data, pmsgq, NULL,
					NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tid);
	k_thread_abort(tid1);

	/**TESTPOINT: msgq purge*/
	k_msgq_purge(pmsgq);
}

static void get_empty_entry(void *p1, void *p2, void *p3)
{
	int ret;
	static uint32_t rx_buf[MSGQ_LEN];

	/* make sure there is no message in the queue */
	ret = k_msgq_peek(p1, rx_buf);
	zassert_equal(ret, -ENOMSG, "Peek message from empty queue");

	ret = k_msgq_get(p1, rx_buf, K_NO_WAIT);
	zassert_equal(ret, -ENOMSG, "Got message from empty queue");

	/* blocked to TIMEOUT */
	ret = k_msgq_get(p1, rx_buf, TIMEOUT);
	zassert_equal(ret, -EAGAIN, "Got message from empty queue");

	k_sem_give(&end_sema);
	/* blocked forever */
	ret = k_msgq_get(p1, rx_buf, K_FOREVER);
	zassert_equal(ret, 0, NULL);
}

static void put_full_entry(void *p1, void *p2, void *p3)
{
	int ret;

	/* make sure the queue is full */
	zassert_equal(k_msgq_num_free_get(p1), 0, NULL);
	zassert_equal(k_msgq_num_used_get(p1), 1, NULL);

	ret = k_msgq_put(p1, &data[1], K_NO_WAIT);
	zassert_equal(ret, -ENOMSG, "Put message to full queue");

	/* blocked to TIMEOUT */
	ret = k_msgq_put(p1, &data[1], TIMEOUT);
	zassert_equal(ret, -EAGAIN, "Put message to full queue");

	k_sem_give(&end_sema);
	/* blocked forever */
	ret = k_msgq_put(p1, &data[1], K_FOREVER);
	zassert_equal(ret, 0, NULL);
}

/**
 * @addtogroup kernel_message_queue_tests
 * @{
 */

/**
 * @brief Test thread to thread data passing via message queue
 * @see k_msgq_init(), k_msgq_get(), k_msgq_put(), k_msgq_purge()
 */
void test_msgq_thread(void)
{
	int ret;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0, NULL);

	msgq_thread(&msgq);
	msgq_thread(&kmsgq);
}

/**
 * @brief Test thread to thread data passing via message queue
 * @see k_msgq_init(), k_msgq_get(), k_msgq_put(), k_msgq_purge()
 */
void test_msgq_thread_overflow(void)
{
	int ret;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, 1);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0, NULL);

	msgq_thread_overflow(&msgq);
	msgq_thread_overflow(&kmsgq);
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Test user thread to kernel thread data passing via message queue
 * @see k_msgq_alloc_init(), k_msgq_get(), k_msgq_put(), k_msgq_purge()
 */
void test_msgq_user_thread(void)
{
	struct k_msgq *q;
	int ret;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN), NULL);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0, NULL);

	msgq_thread(q);
}

/**
 * @brief Test thread to thread data passing via message queue
 * @see k_msgq_alloc_init(), k_msgq_get(), k_msgq_put(), k_msgq_purge()
 */
void test_msgq_user_thread_overflow(void)
{
	struct k_msgq *q;
	int ret;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, 1), NULL);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0, NULL);

	msgq_thread_overflow(q);
}
#endif /* CONFIG_USERSPACE */

/**
 * @brief Test thread to isr data passing via message queue
 * @see k_msgq_init(), k_msgq_get(), k_msgq_put(), k_msgq_purge()
 */
void test_msgq_isr(void)
{
	static struct k_msgq stack_msgq;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&stack_msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	msgq_isr(&stack_msgq);
	msgq_isr(&kmsgq);
}

/**
 * @brief Test pending writer in msgq
 * @see k_msgq_init(), k_msgq_get(), k_msgq_put(), k_msgq_purge()
 */
void test_msgq_pend_thread(void)
{
	int ret;

	k_msgq_init(&msgq1, tbuffer1, MSG_SIZE, 1);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0, NULL);

	msgq_thread_data_passing(&msgq1);
}

/**
 * @brief Test k_msgq_alloc_init()
 * @details Initialization and buffer allocation for msgq from resource
 * pool with various parameters
 * @see k_msgq_alloc_init(), k_msgq_cleanup()
 */
void test_msgq_alloc(void)
{
	int ret;

	k_msgq_alloc_init(&kmsgq_test_alloc, MSG_SIZE, MSGQ_LEN);
	msgq_isr(&kmsgq_test_alloc);
	k_msgq_cleanup(&kmsgq_test_alloc);

	/** Requesting buffer allocation from the test pool.*/
	ret = k_msgq_alloc_init(&kmsgq_test_alloc, MSG_SIZE * 128, MSGQ_LEN);
	zassert_true(ret == -ENOMEM,
		"resource pool is smaller then requested buffer");

	/* Requesting a huge size of MSG to validate overflow*/
	ret = k_msgq_alloc_init(&kmsgq_test_alloc, OVERFLOW_SIZE_MSG, MSGQ_LEN);
	zassert_true(ret == -EINVAL, "Invalid request");
}

/**
 * @brief Get message from an empty queue
 *
 * @details
 * - A thread get message from an empty message queue will get a -ENOMSG if
 *   timeout is set to K_NO_WAIT
 * - A thread get message from an empty message queue will be blocked if timeout
 *   is set to a positive value or K_FOREVER
 *
 * @see k_msgq_get()
 */
void test_msgq_empty(void)
{
	int pri = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	k_msgq_init(&msgq1, tbuffer1, MSG_SIZE, 1);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0, NULL);

	k_tid_t tid = k_thread_create(&tdata2, tstack2, STACK_SIZE,
				      get_empty_entry, &msgq1, NULL,
				      NULL, pri, 0, K_NO_WAIT);

	k_sem_take(&end_sema, K_FOREVER);
	/* that getting thread is being blocked now */
	zassert_equal(tid->base.thread_state, _THREAD_PENDING, NULL);
	/* since there is a thread is waiting for message, this queue
	 * can't be cleanup
	 */
	ret = k_msgq_cleanup(&msgq1);
	zassert_equal(ret, -EBUSY, NULL);

	/* put a message to wake that getting thread */
	ret = k_msgq_put(&msgq1, &data[0], K_NO_WAIT);
	zassert_equal(ret, 0, NULL);

	k_thread_abort(tid);
}

/**
 * @brief Put message to a full queue
 *
 * @details
 * - A thread put message to a full message queue will get a -ENOMSG if
 *   timeout is set to K_NO_WAIT
 * - A thread put message to a full message queue will be blocked if timeout
 *   is set to a positive value or K_FOREVER
 *
 * @see k_msgq_put()
 */
void test_msgq_full(void)
{
	int pri = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	k_msgq_init(&msgq1, tbuffer1, MSG_SIZE, 1);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0, NULL);

	ret = k_msgq_put(&msgq1, &data[0], K_NO_WAIT);
	zassert_equal(ret, 0, NULL);

	k_tid_t tid = k_thread_create(&tdata2, tstack2, STACK_SIZE,
					put_full_entry, &msgq1, NULL,
					NULL, pri, 0, K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);
	/* that putting thread is being blocked now */
	zassert_equal(tid->base.thread_state, _THREAD_PENDING, NULL);
	k_thread_abort(tid);
}

/**
 * @}
 */
