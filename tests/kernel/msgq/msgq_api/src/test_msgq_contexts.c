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
ZTEST_BMEM k_tid_t tids[2];
struct k_thread tdata;
struct k_thread tdata1;
struct k_thread tdata2;
static ZTEST_BMEM char __aligned(4) tbuffer[MSG_SIZE * MSGQ_LEN];
static ZTEST_DMEM char __aligned(4) tbuffer1[MSG_SIZE];
static ZTEST_DMEM uint32_t data[MSGQ_LEN] = { MSG0, MSG1 };
static ZTEST_DMEM uint32_t msg3 = 0x2345;
struct k_sem end_sema;

static void put_msgq(struct k_msgq *pmsgq)
{
	int ret;
	uint32_t read_data;

	for (int i = 0; i < MSGQ_LEN; i++) {
		ret = IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ?
					k_msgq_put_front(pmsgq, (void *) &data[i]) :
					k_msgq_put(pmsgq, (void *)&data[i], K_NO_WAIT);
		zassert_equal(ret, 0);

		/**TESTPOINT: Check if k_msgq_peek reads msgq.
		 */
		zassert_equal(k_msgq_peek(pmsgq, &read_data), 0);
		zassert_equal(read_data, IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ?
			data[i] : data[0]);

		/**TESTPOINT: msgq free get*/
		zassert_equal(k_msgq_num_free_get(pmsgq),
				MSGQ_LEN - 1 - i, NULL);
		/**TESTPOINT: msgq used get*/
		zassert_equal(k_msgq_num_used_get(pmsgq), i + 1);
	}
}

static void get_msgq(struct k_msgq *pmsgq)
{
	uint32_t rx_data, read_data;
	int ret;

	for (int i = 0; i < MSGQ_LEN; i++) {
		zassert_equal(k_msgq_peek(pmsgq, &read_data), 0);

		ret = k_msgq_get(pmsgq, &rx_data, K_FOREVER);
		zassert_equal(ret, 0);
		zassert_equal(rx_data,
			IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ? data[MSGQ_LEN - i - 1] : data[i]);

		/**TESTPOINT: Check if msg read is the msg deleted*/
		zassert_equal(read_data, rx_data);
		/**TESTPOINT: msgq free get*/
		zassert_equal(k_msgq_num_free_get(pmsgq), i + 1);
		/**TESTPOINT: msgq used get*/
		zassert_equal(k_msgq_num_used_get(pmsgq),
				MSGQ_LEN - 1 - i, NULL);
	}
}

static void purge_msgq(struct k_msgq *pmsgq)
{
	uint32_t read_data;

	k_msgq_purge(pmsgq);
	zassert_equal(k_msgq_num_free_get(pmsgq), MSGQ_LEN);
	zassert_equal(k_msgq_num_used_get(pmsgq), 0);
	zassert_equal(k_msgq_peek(pmsgq, &read_data), -ENOMSG);
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
	tids[0] = k_thread_create(&tdata, tstack, STACK_SIZE,
				  thread_entry, pmsgq, NULL, NULL,
				  K_PRIO_PREEMPT(0),
				  K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tids[0]);

	/**TESTPOINT: msgq purge*/
	purge_msgq(pmsgq);
}

static void thread_entry_overflow(void *p1, void *p2, void *p3)
{
	int ret;

	uint32_t rx_buf[MSGQ_LEN];

	ret = k_msgq_get(p1, &rx_buf[0], K_FOREVER);

	zassert_equal(ret, 0);

	ret = k_msgq_get(p1, &rx_buf[1], K_FOREVER);

	zassert_equal(ret, 0);

	k_sem_give(&end_sema);
}

static void msgq_thread_overflow(struct k_msgq *pmsgq)
{
	int ret;

	ret = k_msgq_put(pmsgq, (void *)&data[0], K_FOREVER);

	zassert_equal(ret, 0);

	/**TESTPOINT: thread-thread data passing via message queue*/
	tids[0] = k_thread_create(&tdata, tstack, STACK_SIZE,
				  thread_entry_overflow, pmsgq, NULL, NULL,
				  K_PRIO_PREEMPT(0),
				  K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	ret = k_msgq_put(pmsgq, (void *)&data[1], K_FOREVER);
	zassert_equal(ret, 0);

	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tids[0]);

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

	ret = IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ?
		k_msgq_put_front(p1, &data[1]) :
		k_msgq_put(p1, &data[1], TIMEOUT);
	zassert_equal(ret, IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ? -ENOMSG : 0);
}

static void msgq_thread_data_passing(struct k_msgq *pmsgq)
{
	while (
		IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ?
			k_msgq_put_front(pmsgq, &data[0]) != 0 :
			k_msgq_put(pmsgq, &data[0], K_NO_WAIT) != 0
		) {
	}

	tids[0] = k_thread_create(&tdata2, tstack2, STACK_SIZE,
				  pend_thread_entry, pmsgq, NULL,
				  NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	tids[1] = k_thread_create(&tdata1, tstack1, STACK_SIZE,
				  thread_entry_get_data, pmsgq, NULL,
				  NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	k_sem_take(&end_sema, K_FOREVER);
	k_thread_abort(tids[0]);
	k_thread_abort(tids[1]);

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
	zassert_equal(ret, 0);
}

static void put_full_entry(void *p1, void *p2, void *p3)
{
	int ret;

	/* make sure the queue is full */
	zassert_equal(k_msgq_num_free_get(p1), 0);
	zassert_equal(k_msgq_num_used_get(p1), 1);

	ret = k_msgq_put(p1, &data[1], K_NO_WAIT);
	zassert_equal(ret, -ENOMSG, "Put message to full queue");

	/* blocked to TIMEOUT */
	ret = k_msgq_put(p1, &data[1], TIMEOUT);
	zassert_equal(ret, -EAGAIN, "Put message to full queue");

	k_sem_give(&end_sema);
	/* blocked forever */
	ret = k_msgq_put(p1, &data[1], K_FOREVER);
	zassert_equal(ret, 0);
}

static void prepend_full_entry(void *p1, void *p2, void *p3)
{
	int ret;

	/* make sure the queue is full */
	zassert_equal(k_msgq_num_free_get(p1), 0);
	zassert_equal(k_msgq_num_used_get(p1), 2);
	k_sem_give(&end_sema);

	/* prepend a new message */
	ret = IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ? k_msgq_put_front(p1, &msg3) :
		k_msgq_put(p1, &msg3, K_FOREVER);
	zassert_equal(ret, IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ? -ENOMSG : 0);
}

/**
 * @addtogroup tests_kernel_msgq
 * @{
 */

/**
 * @brief Verify FIFO data passing through a message queue between two threads.
 *
 * @details
 * A producer fills the queue and a consumer thread drains it; messages must be
 * delivered in FIFO order and the occupancy counters (num_free/num_used) and
 * k_msgq_peek() must track the contents throughout. Run against both a
 * k_msgq_init()ed and a K_MSGQ_DEFINE()d queue.
 *
 * Test steps:
 * - Producer puts MSGQ_LEN messages, checking peek and free/used counts.
 * - A consumer thread gets them all and verifies FIFO order.
 * - Purge the queue and confirm it is empty.
 *
 * Expected result:
 * - All messages are received in order and the counters stay consistent.
 *
 * @note The kernel.message_queue.put_front scenario builds this with
 *       CONFIG_TEST_MSGQ_PUT_FRONT=y, exercising k_msgq_put_front() in place of
 *       k_msgq_put() and checking the resulting prepend (LIFO) ordering.
 *
 * @see k_msgq_put()
 * @see k_msgq_put_front()
 * @see k_msgq_get()
 */
ZTEST(msgq_api_1cpu, test_msgq_thread)
{
	int ret;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, MSGQ_LEN);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	msgq_thread(&msgq);
	msgq_thread(&kmsgq);
}

/**
 * @brief Verify the ring buffer wraps correctly across put/get cycles.
 *
 * @details
 * After messages have been consumed, new puts must reuse freed slots and the
 * internal write pointer must advance (wrap) rather than reset to the buffer
 * start. A blocked consumer and interleaved puts exercise the wrap, and the test
 * asserts the write pointer did not return to the buffer start.
 *
 * Test steps:
 * - Initialize a length-2 queue and put one message.
 * - Start a consumer that gets two messages, then put the second message.
 * - After the exchange, verify the write pointer is not at buffer_start.
 *
 * Expected result:
 * - Messages pass correctly and the write pointer wraps within the ring buffer.
 *
 * @see k_msgq_put()
 * @see k_msgq_get()
 */
ZTEST(msgq_api, test_msgq_thread_overflow)
{
	int ret;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&msgq, tbuffer, MSG_SIZE, 2);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	ret = k_msgq_put(&msgq, (void *)&data[0], K_FOREVER);
	zassert_equal(ret, 0);

	msgq_thread_overflow(&msgq);
	msgq_thread_overflow(&kmsgq);

	/*verify the write pointer not reset to the buffer start*/
	zassert_false(msgq.write_ptr == msgq.buffer_start,
		"Invalid add operation of message queue");
}

#ifdef CONFIG_USERSPACE
/**
 * @brief Verify thread-to-thread message passing from user mode.
 *
 * @details
 * Same FIFO data-passing contract as test_msgq_thread(), exercised from a
 * user-mode thread on a queue created with k_msgq_alloc_init(), confirming the
 * put/get/purge flow works under userspace.
 *
 * Test steps:
 * - Allocate and alloc-init a queue as a user thread.
 * - Run the producer/consumer data-passing sequence and purge.
 *
 * Expected result:
 * - Messages pass in order from user mode.
 *
 * @see k_msgq_alloc_init()
 * @see k_msgq_put()
 * @see k_msgq_get()
 */
ZTEST_USER(msgq_api, test_msgq_user_thread)
{
	struct k_msgq *q;
	int ret;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, MSGQ_LEN));
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	msgq_thread(q);
}

/**
 * @brief Verify ring-buffer wrap from user mode.
 *
 * @details
 * User-mode counterpart of test_msgq_thread_overflow(): on a length-1 queue
 * created with k_msgq_alloc_init(), interleaved puts and gets must reuse freed
 * slots correctly from userspace.
 *
 * Test steps:
 * - Allocate and alloc-init a length-1 queue as a user thread.
 * - Run the overflow put/get sequence with a blocked consumer.
 *
 * Expected result:
 * - Messages pass correctly with buffer wrap from user mode.
 *
 * @see k_msgq_alloc_init()
 * @see k_msgq_put()
 * @see k_msgq_get()
 */
ZTEST_USER(msgq_api, test_msgq_user_thread_overflow)
{
	struct k_msgq *q;
	int ret;

	q = k_object_alloc(K_OBJ_MSGQ);
	zassert_not_null(q, "couldn't alloc message queue");
	zassert_false(k_msgq_alloc_init(q, MSG_SIZE, 1));
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	msgq_thread_overflow(q);
}
#endif /* CONFIG_USERSPACE */

/**
 * @brief Verify message passing from an ISR to a thread.
 *
 * @details
 * Messages enqueued from interrupt context (via irq_offload()) must be retrieved
 * in thread context in FIFO order, confirming k_msgq_put() is ISR-safe. Run
 * against both an init()ed and a K_MSGQ_DEFINE()d queue.
 *
 * Test steps:
 * - From an ISR, put MSGQ_LEN messages into the queue.
 * - In thread context, get them all and verify FIFO order, then purge.
 *
 * Expected result:
 * - The thread receives every ISR-enqueued message in order.
 *
 * @note The kernel.message_queue.put_front scenario builds this with
 *       CONFIG_TEST_MSGQ_PUT_FRONT=y, so the ISR enqueues via k_msgq_put_front()
 *       and the thread checks the prepend (LIFO) ordering.
 *
 * @see k_msgq_put()
 * @see k_msgq_put_front()
 * @see k_msgq_get()
 */
ZTEST(msgq_api, test_msgq_isr)
{
	static struct k_msgq stack_msgq;

	/**TESTPOINT: init via k_msgq_init*/
	k_msgq_init(&stack_msgq, tbuffer, MSG_SIZE, MSGQ_LEN);

	msgq_isr(&stack_msgq);
	msgq_isr(&kmsgq);
}

/**
 * @brief Verify a writer pending on a full queue is served when space frees.
 *
 * @details
 * On a length-1 queue, a writer that blocks because the queue is full must be
 * unblocked and complete its put once a reader removes a message, passing the
 * data through to the reader.
 *
 * Test steps:
 * - Fill a length-1 queue, then start a writer that pends on a put.
 * - Start a reader that drains the queue.
 * - Confirm the pending put completes and the data is delivered, then purge.
 *
 * Expected result:
 * - The blocked writer is woken and its message reaches the reader.
 *
 * @note Under the kernel.message_queue.put_front scenario
 *       (CONFIG_TEST_MSGQ_PUT_FRONT=y) the writer uses k_msgq_put_front(), which
 *       does not block: on a full queue it returns -ENOMSG instead of pending.
 *
 * @see k_msgq_put()
 * @see k_msgq_put_front()
 * @see k_msgq_get()
 */
ZTEST(msgq_api_1cpu, test_msgq_pend_thread)
{
	int ret;

	k_msgq_init(&msgq1, tbuffer1, MSG_SIZE, 1);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	msgq_thread_data_passing(&msgq1);
}

/**
 * @brief Verify k_msgq_alloc_init() allocation success and failure paths.
 *
 * @details
 * k_msgq_alloc_init() draws its buffer from the thread's resource pool. It must
 * succeed for a request the pool can satisfy, return -ENOMEM when the requested
 * buffer exceeds the pool, and -EINVAL when the message size would overflow.
 *
 * Test steps:
 * - Alloc-init a queue that fits the pool and use it, then clean it up.
 * - Request a buffer larger than the pool and expect -ENOMEM.
 * - Request an overflowing message size and expect -EINVAL.
 *
 * Expected result:
 * - Success for a valid request; -ENOMEM and -EINVAL for the failing requests.
 *
 * @see k_msgq_alloc_init()
 * @see k_msgq_cleanup()
 */
ZTEST(msgq_api, test_msgq_alloc)
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
 * @brief Verify get-from-empty timeout behavior and cleanup-while-pending.
 *
 * @details
 * A get on an empty queue must return -ENOMSG for K_NO_WAIT, -EAGAIN after a
 * finite timeout, and block under K_FOREVER until a put arrives. While a thread
 * is pending on the queue, k_msgq_cleanup() must refuse with -EBUSY.
 *
 * Test steps:
 * - Start a thread that gets from an empty queue with K_NO_WAIT (-ENOMSG),
 *   a finite timeout (-EAGAIN), then blocks on K_FOREVER.
 * - While it is pending, call k_msgq_cleanup() and expect -EBUSY.
 * - Put a message to wake the pending getter.
 *
 * Expected result:
 * - The get returns -ENOMSG/-EAGAIN as appropriate, cleanup returns -EBUSY, and
 *   the K_FOREVER get completes once a message is put.
 *
 * @see k_msgq_get()
 * @see k_msgq_cleanup()
 */
ZTEST(msgq_api_1cpu, test_msgq_empty)
{
	int pri = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	k_msgq_init(&msgq1, tbuffer1, MSG_SIZE, 1);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	tids[0] = k_thread_create(&tdata2, tstack2, STACK_SIZE,
				  get_empty_entry, &msgq1, NULL,
				  NULL, pri, 0, K_NO_WAIT);

	k_sem_take(&end_sema, K_FOREVER);
	/* that getting thread is being blocked now */
	zassert_equal(tids[0]->base.thread_state, _THREAD_PENDING);
	/* since there is a thread is waiting for message, this queue
	 * can't be cleanup
	 */
	ret = k_msgq_cleanup(&msgq1);
	zassert_equal(ret, -EBUSY);

	/* put a message to wake that getting thread */
	ret = k_msgq_put(&msgq1, &data[0], K_NO_WAIT);
	zassert_equal(ret, 0);

	k_thread_abort(tids[0]);
}

/**
 * @brief Verify put-to-full timeout behavior.
 *
 * @details
 * A put on a full queue must return -ENOMSG for K_NO_WAIT, -EAGAIN after a
 * finite timeout, and block under K_FOREVER until space becomes available.
 *
 * Test steps:
 * - Fill a length-1 queue, then start a thread that puts with K_NO_WAIT
 *   (-ENOMSG), a finite timeout (-EAGAIN), then blocks on K_FOREVER.
 * - Confirm the thread is pending after the non-blocking attempts.
 *
 * Expected result:
 * - The put returns -ENOMSG/-EAGAIN as appropriate and blocks under K_FOREVER.
 *
 * @see k_msgq_put()
 */
ZTEST(msgq_api_1cpu, test_msgq_full)
{
	int pri = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	k_msgq_init(&msgq1, tbuffer1, MSG_SIZE, 1);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	ret = k_msgq_put(&msgq1, &data[0], K_NO_WAIT);
	zassert_equal(ret, 0);

	tids[0] = k_thread_create(&tdata2, tstack2, STACK_SIZE,
				  put_full_entry, &msgq1, NULL,
				  NULL, pri, 0, K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);
	/* that putting thread is being blocked now */
	zassert_equal(tids[0]->base.thread_state, _THREAD_PENDING);
	k_thread_abort(tids[0]);
}

/**
 * @brief Verify message ordering when a writer pends on a full queue.
 *
 * @details
 * With a full length-2 queue, a thread attempting another put (or put_front)
 * blocks; once the main thread drains the queue, the messages must come out in
 * the expected order, validating that pending writers do not corrupt ordering.
 *
 * Test steps:
 * - Fill a length-2 queue (using put and put_front).
 * - Start a thread that attempts a further prepend/put and pends.
 * - Drain the queue and verify the messages are returned in the expected order.
 *
 * Expected result:
 * - Messages are dequeued in the correct order despite the pending writer.
 *
 * @note The kernel.message_queue.put_front scenario
 *       (CONFIG_TEST_MSGQ_PUT_FRONT=y) fills the queue and prepends via
 *       k_msgq_put_front(), checking that a full-queue prepend returns -ENOMSG
 *       and that the front-inserted message is dequeued first.
 *
 * @see k_msgq_put()
 * @see k_msgq_put_front()
 */
ZTEST(msgq_api_1cpu, test_msgq_thread_pending)
{
	uint32_t rx_data;
	int pri = k_thread_priority_get(k_current_get()) - 1;
	int ret;

	k_msgq_init(&msgq1, tbuffer, MSG_SIZE, 2);
	ret = k_sem_init(&end_sema, 0, 1);
	zassert_equal(ret, 0);

	ret = IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ?
		k_msgq_put_front(&msgq1, &data[1]) :
		k_msgq_put(&msgq1, &data[0], K_NO_WAIT);
	zassert_equal(ret, 0);
	ret = IS_ENABLED(CONFIG_TEST_MSGQ_PUT_FRONT) ?
		k_msgq_put(&msgq1, &data[0], K_NO_WAIT) :
		k_msgq_put_front(&msgq1, &data[1]);
	zassert_equal(ret, 0);

	k_tid_t tid = k_thread_create(&tdata2, tstack2, STACK_SIZE,
					prepend_full_entry, &msgq1, NULL,
					NULL, pri, 0, K_NO_WAIT);

	/* that putting thread is being blocked now */
	k_sem_take(&end_sema, K_FOREVER);

	ret = k_msgq_get(&msgq1, &rx_data, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(rx_data, data[1]);

	ret = k_msgq_get(&msgq1, &rx_data, K_FOREVER);
	zassert_equal(ret, 0);
	zassert_equal(rx_data, data[0]);
	k_thread_abort(tid);
}

/**
 * @brief Test peeking at a message by index without removing it
 *
 * @details Put two distinct messages into a message queue, then use
 * k_msgq_peek_at() to read the message at each valid index and verify the
 * returned values match what was enqueued, without removing them (the used
 * count stays unchanged). Also verify that peeking at an index beyond the
 * number of queued messages returns -ENOMSG.
 *
 * @see k_msgq_peek_at()
 */
ZTEST_USER(msgq_api, test_msgq_peek_at)
{
	uint32_t read_data;

	k_msgq_purge(&kmsgq);

	zassert_equal(k_msgq_put(&kmsgq, &data[0], K_NO_WAIT), 0);
	zassert_equal(k_msgq_put(&kmsgq, &data[1], K_NO_WAIT), 0);

	/* Peek at each index; messages are queued FIFO so index 0 is oldest. */
	zassert_equal(k_msgq_peek_at(&kmsgq, &read_data, 0), 0);
	zassert_equal(read_data, data[0]);
	zassert_equal(k_msgq_peek_at(&kmsgq, &read_data, 1), 0);
	zassert_equal(read_data, data[1]);

	/* Peeking must not remove any message. */
	zassert_equal(k_msgq_num_used_get(&kmsgq), MSGQ_LEN);

	/* An index at or beyond the number of queued messages returns -ENOMSG. */
	zassert_equal(k_msgq_peek_at(&kmsgq, &read_data, MSGQ_LEN), -ENOMSG);

	k_msgq_purge(&kmsgq);
}

/**
 * @}
 */
