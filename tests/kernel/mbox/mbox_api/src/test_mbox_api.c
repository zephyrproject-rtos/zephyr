/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define TIMEOUT K_MSEC(100)
#if !defined(CONFIG_BOARD_QEMU_X86)
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#else
#define STACK_SIZE (640 + CONFIG_TEST_EXTRA_STACK_SIZE)
#endif
#define MAIL_LEN 64
/* Receiver buffer smaller than the sent message, to exercise size negotiation. */
#define TRUNCATED_SIZE 16

/**
 * @brief Tests for the mailbox kernel object
 * @defgroup tests_kernel_mbox Mailbox tests
 * @ingroup all_tests
 * @{
 * @}
 */

/**TESTPOINT: init via K_MBOX_DEFINE*/
K_MBOX_DEFINE(kmbox);

static struct k_mbox mbox;

static k_tid_t sender_tid, receiver_tid, random_tid;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack_1, STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(waiting_get_stack, 5, STACK_SIZE);
static struct k_thread tdata, async_tid, waiting_get_tid[5];

static struct k_sem end_sema, sync_sema;

static enum mmsg_type {
	PUT_GET_NULL = 0,
	PUT_GET_BUFFER,
	ASYNC_PUT_GET_BUFFER,
	ASYNC_PUT_GET_BLOCK,
	TARGET_SOURCE_THREAD_BUFFER,
	MAX_INFO_TYPE,
	INCORRECT_RECEIVER_TID,
	INCORRECT_TRANSMIT_TID,
	TIMED_OUT_MBOX_GET,
	MSG_TID_MISMATCH,
	DISPOSE_SIZE_0_MSG,
	ASYNC_PUT_TO_WAITING_GET,
	GET_WAITING_PUT_INCORRECT_TID,
	ASYNC_MULTIPLE_PUT,
	MULTIPLE_WAITING_GET
} info_type;

static char data[MAX_INFO_TYPE][MAIL_LEN] = {
	"send/recv an empty message",
	"send/recv msg using a buffer",
	"async send/recv msg using a memory block",
	"specify target/source thread, using a memory block"
};

static void async_put_sema_give(void *p1, void *p2, void *p3)
{
	k_sem_give(&sync_sema);
}


static void mbox_get_waiting_thread(void *p1, void *p2, void *p3)
{
	int thread_number = POINTER_TO_INT(p1);
	struct k_mbox *pmbox = p2;
	struct k_mbox_msg mmsg = {0};

	switch (thread_number) {
	case 0:
		mmsg.rx_source_thread = K_ANY;
		break;

	case 1:
		mmsg.rx_source_thread = random_tid;
		break;

	case 2:
		mmsg.rx_source_thread = receiver_tid;
		break;

	case 3:
		mmsg.rx_source_thread = &async_tid;
		break;

	case 4:
		mmsg.rx_source_thread = K_ANY;
		break;

	default:
		break;
	}

	mmsg.size = 0;
	zassert_true(k_mbox_get(pmbox, &mmsg, NULL, K_FOREVER) == 0,
		     "Failure at thread number %d", thread_number);

}

static void tmbox_put(struct k_mbox *pmbox)
{
	struct k_mbox_msg mmsg = {0};

	switch (info_type) {
	case PUT_GET_NULL:
		/**TESTPOINT: mbox sync put empty message*/
		mmsg.info = PUT_GET_NULL;
		mmsg.size = 0;
		mmsg.tx_data = NULL;
		mmsg.tx_target_thread = K_ANY;
		k_mbox_put(pmbox, &mmsg, K_FOREVER);
		break;
	case PUT_GET_BUFFER:
		__fallthrough;
	case TARGET_SOURCE_THREAD_BUFFER:
		/**TESTPOINT: mbox sync put buffer*/
		mmsg.info = PUT_GET_BUFFER;
		mmsg.size = sizeof(data[info_type]);
		mmsg.tx_data = data[info_type];
		if (info_type == TARGET_SOURCE_THREAD_BUFFER) {
			mmsg.tx_target_thread = receiver_tid;
		} else {
			mmsg.tx_target_thread = K_ANY;
		}
		k_mbox_put(pmbox, &mmsg, K_FOREVER);
		break;
	case ASYNC_PUT_GET_BUFFER:
		/**TESTPOINT: mbox async put buffer*/
		mmsg.info = ASYNC_PUT_GET_BUFFER;
		mmsg.size = sizeof(data[info_type]);
		mmsg.tx_data = data[info_type];
		mmsg.tx_target_thread = K_ANY;
		k_mbox_async_put(pmbox, &mmsg, &sync_sema);
		/*wait for msg being taken*/
		k_sem_take(&sync_sema, K_FOREVER);
		break;
	case ASYNC_PUT_GET_BLOCK:
		/* Offer a full buffer to a receiver that accepts fewer bytes.
		 * The kernel must truncate the transfer and report the
		 * negotiated (smaller) size back to the sender.
		 */
		mmsg.info = PUT_GET_BUFFER;
		mmsg.size = sizeof(data[ASYNC_PUT_GET_BLOCK]);
		mmsg.tx_data = data[ASYNC_PUT_GET_BLOCK];
		mmsg.tx_target_thread = K_ANY;
		k_mbox_put(pmbox, &mmsg, K_FOREVER);
		/**TESTPOINT: sender learns the actually transferred size*/
		zassert_equal(mmsg.size, TRUNCATED_SIZE,
			      "sender size not negotiated down: %zu", mmsg.size);
		break;
	case INCORRECT_TRANSMIT_TID:
		mmsg.tx_target_thread = random_tid;
		zassert_true(k_mbox_put(pmbox,
					&mmsg,
					K_NO_WAIT) == -ENOMSG, NULL);
		break;
	case MSG_TID_MISMATCH:
		/* keep one msg in the queue and try to get with a wrong tid */
		mmsg.info = PUT_GET_NULL;
		mmsg.size = 0;
		mmsg.tx_data = NULL;
		mmsg.tx_target_thread = sender_tid;
		/* timeout because this msg won't be received with a _get*/
		k_mbox_put(pmbox, &mmsg, TIMEOUT);
		break;

	case DISPOSE_SIZE_0_MSG:
		/* Get a msg and dispose it by making the size = 0 */
		mmsg.size = 0;
		mmsg.tx_data = data[1];
		mmsg.tx_target_thread = K_ANY;
		zassert_true(k_mbox_put(pmbox, &mmsg, K_FOREVER) == 0);
		break;

	case ASYNC_PUT_TO_WAITING_GET:
		k_sem_take(&sync_sema, K_FOREVER);
		mmsg.size = sizeof(data[0]);
		mmsg.tx_data = data[0];
		mmsg.tx_target_thread = K_ANY;
		k_mbox_async_put(pmbox, &mmsg, NULL);
		break;
	case GET_WAITING_PUT_INCORRECT_TID:
		k_sem_take(&sync_sema, K_FOREVER);
		mmsg.size = sizeof(data[0]);
		mmsg.tx_data = data[0];
		mmsg.tx_target_thread = random_tid;
		k_mbox_async_put(pmbox, &mmsg, &sync_sema);
		break;
	case ASYNC_MULTIPLE_PUT:
		mmsg.size = sizeof(data[0]);
		mmsg.tx_data = data[0];
		mmsg.tx_target_thread = K_ANY;
		k_mbox_async_put(pmbox, &mmsg, NULL);

		mmsg.tx_data = data[1];
		mmsg.tx_target_thread = &async_tid;
		k_mbox_async_put(pmbox, &mmsg, NULL);

		mmsg.tx_data = data[1];
		mmsg.tx_target_thread = receiver_tid;
		k_mbox_async_put(pmbox, &mmsg, NULL);

		mmsg.tx_data = data[1];
		mmsg.tx_target_thread = &async_tid;
		k_mbox_async_put(pmbox, &mmsg, NULL);

		mmsg.tx_data = data[2];
		mmsg.tx_target_thread = receiver_tid;
		k_mbox_async_put(pmbox, &mmsg, &sync_sema);

		k_sem_take(&sync_sema, K_FOREVER);
		break;

	case MULTIPLE_WAITING_GET:
		k_sem_take(&sync_sema, K_FOREVER);

		mmsg.size = sizeof(data[0]);
		mmsg.tx_data = data[0];
		mmsg.tx_target_thread = K_ANY;
		k_mbox_put(pmbox, &mmsg, K_NO_WAIT);

		mmsg.tx_data = data[1];
		mmsg.tx_target_thread = &async_tid;
		k_mbox_put(pmbox, &mmsg, K_NO_WAIT);

		mmsg.tx_data = data[1];
		mmsg.tx_target_thread = receiver_tid;
		k_mbox_put(pmbox, &mmsg, K_NO_WAIT);

		mmsg.tx_data = data[1];
		mmsg.tx_target_thread = &async_tid;
		k_mbox_put(pmbox, &mmsg, K_NO_WAIT);

		mmsg.tx_data = data[2];
		mmsg.tx_target_thread = receiver_tid;
		k_mbox_put(pmbox, &mmsg, K_NO_WAIT);

		break;
	default:
		break;
	}
}

static void tmbox_get(struct k_mbox *pmbox)
{
	struct k_mbox_msg mmsg = {0};
	char rxdata[MAIL_LEN];

	switch (info_type) {
	case PUT_GET_NULL:
		/**TESTPOINT: mbox sync get buffer*/
		mmsg.size = sizeof(rxdata);
		mmsg.rx_source_thread = K_ANY;
		/*verify return value*/
		zassert_true(k_mbox_get(pmbox, &mmsg, rxdata, K_FOREVER) == 0,
			     NULL);
		/*verify .info*/
		zassert_equal(mmsg.info, PUT_GET_NULL);
		/*verify .size*/
		zassert_equal(mmsg.size, 0);
		break;
	case PUT_GET_BUFFER:
		__fallthrough;
	case TARGET_SOURCE_THREAD_BUFFER:
		/**TESTPOINT: mbox sync get buffer*/
		mmsg.size = sizeof(rxdata);
		if (info_type == TARGET_SOURCE_THREAD_BUFFER) {
			mmsg.rx_source_thread = sender_tid;
		} else {
			mmsg.rx_source_thread = K_ANY;
		}
		zassert_true(k_mbox_get(pmbox, &mmsg, rxdata, K_FOREVER) == 0,
			     NULL);
		zassert_equal(mmsg.info, PUT_GET_BUFFER);
		zassert_equal(mmsg.size, sizeof(data[info_type]));
		/*verify rxdata*/
		zassert_true(memcmp(rxdata, data[info_type], MAIL_LEN) == 0,
			     NULL);
		break;
	case ASYNC_PUT_GET_BUFFER:
		/**TESTPOINT: mbox async get buffer*/
		mmsg.size = sizeof(rxdata);
		mmsg.rx_source_thread = K_ANY;
		zassert_true(k_mbox_get(pmbox, &mmsg, NULL, K_FOREVER) == 0,
			     NULL);
		zassert_equal(mmsg.info, ASYNC_PUT_GET_BUFFER);
		zassert_equal(mmsg.size, sizeof(data[info_type]));
		k_mbox_data_get(&mmsg, rxdata);
		zassert_true(memcmp(rxdata, data[info_type], MAIL_LEN) == 0,
			     NULL);
		break;
	case ASYNC_PUT_GET_BLOCK:
		/* Accept fewer bytes than were sent: the message must be
		 * truncated to the receiver's size and only that prefix copied.
		 */
		mmsg.size = TRUNCATED_SIZE;
		mmsg.rx_source_thread = K_ANY;
		zassert_equal(k_mbox_get(pmbox, &mmsg, rxdata, K_FOREVER), 0,
			      NULL);
		/**TESTPOINT: message truncated to the receiver's size*/
		zassert_equal(mmsg.size, TRUNCATED_SIZE,
			      "receiver size not truncated: %zu", mmsg.size);
		zassert_true(memcmp(rxdata, data[ASYNC_PUT_GET_BLOCK],
				    TRUNCATED_SIZE) == 0, NULL);
		break;
	case INCORRECT_RECEIVER_TID:
		mmsg.rx_source_thread = random_tid;
		zassert_true(k_mbox_get
			     (pmbox, &mmsg, NULL, K_NO_WAIT) == -ENOMSG,
			     NULL);
		break;
	case TIMED_OUT_MBOX_GET:
		mmsg.rx_source_thread = random_tid;
		zassert_true(k_mbox_get(pmbox, &mmsg, NULL, TIMEOUT) == -EAGAIN,
			     NULL);
		break;
	case MSG_TID_MISMATCH:
		mmsg.rx_source_thread = random_tid;
		zassert_true(k_mbox_get
			     (pmbox, &mmsg, NULL, K_NO_WAIT) == -ENOMSG, NULL);
		break;

	case DISPOSE_SIZE_0_MSG:
		mmsg.rx_source_thread = K_ANY;
		mmsg.size = 0;
		zassert_true(k_mbox_get(pmbox, &mmsg, &rxdata, K_FOREVER) == 0,
			     NULL);
		break;

	case ASYNC_PUT_TO_WAITING_GET:

		/* Create a new thread to trigger the semaphore needed for the
		 * async put.
		 */
		k_thread_create(&async_tid, tstack_1, STACK_SIZE,
				       async_put_sema_give, NULL, NULL, NULL,
				       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
		mmsg.rx_source_thread = K_ANY;
		mmsg.size = 0;
		/* Here get is blocked until the thread we created releases
		 *  the semaphore and the async put completes it operation.
		 */
		zassert_true(k_mbox_get(pmbox, &mmsg, NULL, K_FOREVER) == 0,
			     NULL);
		break;
	case GET_WAITING_PUT_INCORRECT_TID:
		/* Create a new thread to trigger the semaphore needed for the
		 * async put.
		 */
		k_thread_create(&async_tid, tstack_1, STACK_SIZE,
				       async_put_sema_give, NULL, NULL, NULL,
				       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
		mmsg.rx_source_thread = &async_tid;
		mmsg.size = 0;
		/* Here the get is waiting for a async put to complete
		 * but the TIDs of the msgs doesn't match and hence
		 * causing a timeout.
		 */
		zassert_true(k_mbox_get(pmbox, &mmsg, NULL, TIMEOUT) == -EAGAIN,
			     NULL);
		/* clean up  */
		mmsg.rx_source_thread = K_ANY;
		zassert_true(k_mbox_get(pmbox, &mmsg, NULL, TIMEOUT) == 0,
			     NULL);
		break;

	case ASYNC_MULTIPLE_PUT:
		/* Async put has now populated the msgs. Now retrieve all
		 * the msgs from the mailbox.
		 */
		mmsg.rx_source_thread = K_ANY;
		mmsg.size = 0;
		/* get K_any msg */
		zassert_true(k_mbox_get(pmbox, &mmsg, NULL, TIMEOUT) == 0,
			     NULL);
		/* get the msg specific to receiver_tid */
		mmsg.rx_source_thread = sender_tid;
		zassert_true(k_mbox_get
			     (pmbox, &mmsg, NULL, TIMEOUT) == 0, NULL);

		/* get msg from async or random tid */
		mmsg.rx_source_thread = K_ANY;
		zassert_true(k_mbox_get
			     (pmbox, &mmsg, NULL, TIMEOUT) == 0, NULL);
		break;
	case MULTIPLE_WAITING_GET:
		/* Create 5 threads who will wait on a mbox_get. */
		for (uint32_t i = 0; i < 5; i++) {
			k_thread_create(&waiting_get_tid[i],
					waiting_get_stack[i],
					STACK_SIZE,
					mbox_get_waiting_thread,
					INT_TO_POINTER(i), pmbox, NULL,
					K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
		}
		/* Create a new thread to trigger the semaphore needed for the
		 * async put. This will trigger the start of the msg transfer.
		 */
		k_thread_create(&async_tid, tstack_1, STACK_SIZE,
				       async_put_sema_give, NULL, NULL, NULL,
				       K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
		break;

	default:
		break;
	}
}



/*entry of contexts*/
static void tmbox_entry(void *p1, void *p2, void *p3)
{
	tmbox_get((struct k_mbox *)p1);
	k_sem_give(&end_sema);
}

static void tmbox(struct k_mbox *pmbox)
{
	/*test case setup*/
	k_sem_reset(&end_sema);
	k_sem_reset(&sync_sema);

	/**TESTPOINT: thread-thread data passing via mbox*/
	sender_tid = k_current_get();
	receiver_tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				       tmbox_entry, pmbox, NULL, NULL,
				       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	tmbox_put(pmbox);
	k_sem_take(&end_sema, K_FOREVER);

	/*test case teardown*/
	k_thread_abort(receiver_tid);
}

/**
 * @addtogroup tests_kernel_mbox
 * @{
 */


/**
 * @brief Verify a mailbox can be initialized at runtime.
 *
 * @details
 * k_mbox_init() must prepare a statically allocated k_mbox for use without
 * error. This is the minimal smoke test that runtime initialization completes.
 *
 * Test steps:
 * - Call k_mbox_init() on a k_mbox instance.
 *
 * Expected result:
 * - The mailbox is initialized and ready for put/get operations.
 *
 * @see k_mbox_init()
 * @verifies ZEP-SRS-25-1
 */
ZTEST(mbox_api, test_mbox_kinit)
{
	/**TESTPOINT: init via k_mbox_init*/
	k_mbox_init(&mbox);
}

/**
 * @brief Verify a statically defined mailbox can pass a message.
 *
 * @details
 * A mailbox created with K_MBOX_DEFINE() must be usable without an explicit
 * k_mbox_init(). The test exchanges an empty (size 0) message through the
 * statically defined mailbox between a sender and a receiver thread.
 *
 * Test steps:
 * - Send and receive an empty message through the K_MBOX_DEFINE()d mailbox.
 *
 * Expected result:
 * - The message is delivered, confirming the static mailbox is operational.
 *
 * @see K_MBOX_DEFINE()
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-2
 */
ZTEST(mbox_api, test_mbox_kdefine)
{
	info_type = PUT_GET_NULL;
	tmbox(&kmbox);
}

static ZTEST_BMEM char __aligned(4) buffer[8];

/**
 * @brief Verify the same buffer can be sent via a message queue and a mailbox.
 *
 * @details
 * Demonstrates that an identical data buffer can be transported by both kernel
 * messaging primitives: it is enqueued on a k_msgq and then sent through a
 * mailbox using the asynchronous buffer path, confirming both accept the data.
 *
 * Test steps:
 * - Initialize a message queue and put the data buffer on it (K_NO_WAIT).
 * - Send the same buffer through the mailbox via async put and receive it.
 *
 * Expected result:
 * - The message queue put succeeds and the mailbox transfers the same buffer.
 *
 * @see k_msgq_put()
 * @see k_mbox_async_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-14
 * @verifies ZEP-SRS-25-15
 */
ZTEST(mbox_api, test_mbox_enhanced_capabilities)
{
	info_type = ASYNC_PUT_GET_BUFFER;
	struct k_msgq msgq;

	k_msgq_init(&msgq, buffer, 4, 2);
	/* send buffer with message queue */
	int ret = k_msgq_put(&msgq, &data[info_type], K_NO_WAIT);

	zassert_equal(ret, 0, "message queue put successful");

	/* send same buffer with mailbox */
	tmbox(&mbox);
}

/**
 * @brief Verify multiple independent mailboxes can be used.
 *
 * @details
 * Several mailboxes initialized from the same code must operate independently;
 * a message sent through each is delivered without interference between them.
 *
 * Test steps:
 * - Initialize three separate mailboxes.
 * - Send and receive an empty message through each in turn.
 *
 * Expected result:
 * - Each mailbox independently delivers its message.
 *
 * @see k_mbox_init()
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-16
 */
ZTEST(mbox_api, test_define_multi_mbox)
{
	/**TESTPOINT: init via k_mbox_init*/
	struct k_mbox mbox1, mbox2, mbox3;

	k_mbox_init(&mbox1);
	k_mbox_init(&mbox2);
	k_mbox_init(&mbox3);

	/* verify via send message */
	info_type = PUT_GET_NULL;
	tmbox(&mbox1);
	tmbox(&mbox2);
	tmbox(&mbox3);
}

/**
 * @brief Verify synchronous transfer of an empty mailbox message.
 *
 * @details
 * A zero-length message (no data buffer) must be deliverable: the receiver
 * observes the message with size 0 and the matching info field, using a
 * synchronous k_mbox_put() and k_mbox_get().
 *
 * Test steps:
 * - Sender puts a size-0 message targeted at K_ANY.
 * - Receiver gets from K_ANY and checks the info and that size is 0.
 *
 * Expected result:
 * - The empty message is delivered with size 0 and the expected info value.
 *
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-5
 */
ZTEST(mbox_api, test_mbox_put_get_null)
{
	info_type = PUT_GET_NULL;
	tmbox(&mbox);
}

/**
 * @brief Verify synchronous transfer of a buffered mailbox message.
 *
 * @details
 * A message carrying a data buffer must be delivered intact: the receiver's
 * info, size and copied payload must match what the sender provided, using
 * synchronous put/get.
 *
 * Test steps:
 * - Sender puts a buffer message targeted at K_ANY.
 * - Receiver gets it into a local buffer and compares info, size and contents.
 *
 * Expected result:
 * - The receiver obtains the exact buffer the sender transmitted.
 *
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-3
 * @verifies ZEP-SRS-25-4
 * @verifies ZEP-SRS-25-5
 * @verifies ZEP-SRS-25-9
 */
ZTEST(mbox_api, test_mbox_put_get_buffer)
{
	info_type = PUT_GET_BUFFER;
	tmbox(&mbox);
}

/**
 * @brief Verify asynchronous put delivers a buffered message.
 *
 * @details
 * k_mbox_async_put() must hand a buffer message to a receiver without the
 * sender blocking on delivery; the receiver retrieves the payload with
 * k_mbox_data_get() and it must match what was sent.
 *
 * Test steps:
 * - Sender async-puts a buffer message and waits on the completion semaphore.
 * - Receiver gets the message, copies the data via k_mbox_data_get(), compares.
 *
 * Expected result:
 * - The asynchronously sent buffer is received intact.
 *
 * @see k_mbox_async_put()
 * @see k_mbox_data_get()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-7
 * @verifies ZEP-SRS-25-8
 */
ZTEST(mbox_api, test_mbox_async_put_get_buffer)
{
	info_type = ASYNC_PUT_GET_BUFFER;
	tmbox(&mbox);
}

/**
 * @brief Verify message-size negotiation when the receiver accepts fewer bytes.
 *
 * @details
 * The receiver advertises in rx_msg->size the maximum it will accept. When that
 * is smaller than the sent message, the kernel must truncate the transfer to the
 * receiver's size, copy only that prefix, and report the negotiated size back to
 * both the receiver (rx_msg->size) and the sender (tx_msg->size).
 *
 * Test steps:
 * - Sender puts a full-length buffer message (MAIL_LEN bytes), K_FOREVER.
 * - Receiver gets with rx size set to TRUNCATED_SIZE (< MAIL_LEN).
 * - Check the receiver's size is TRUNCATED_SIZE and the copied prefix matches.
 * - Check the sender's tx size was updated to TRUNCATED_SIZE after the put.
 *
 * Expected result:
 * - Both sizes are negotiated down to TRUNCATED_SIZE and only that many bytes
 *   are transferred.
 *
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-7
 * @verifies ZEP-SRS-25-10
 */
ZTEST(mbox_api, test_mbox_async_put_get_block)
{
	info_type = ASYNC_PUT_GET_BLOCK;
	tmbox(&mbox);
}

/**
 * @brief Verify a buffer message directed to a specific thread pair.
 *
 * @details
 * A message may be addressed to a specific target thread and received by
 * filtering on a specific source thread; the buffer must be delivered on the
 * matching sender/receiver identities.
 *
 * Test steps:
 * - Sender puts a buffer message with tx_target_thread set to the receiver.
 * - Receiver gets filtering on rx_source_thread set to the sender.
 *
 * Expected result:
 * - The directed message is delivered and its contents match.
 *
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-4
 * @verifies ZEP-SRS-25-13
 */
ZTEST(mbox_api, test_mbox_target_source_thread_buffer)
{
	info_type = TARGET_SOURCE_THREAD_BUFFER;
	tmbox(&mbox);
}

/**
 * @brief Verify a get filtering on a non-matching source returns -ENOMSG.
 *
 * @details
 * A non-blocking get that filters on a specific source thread which has no
 * pending message must fail with -ENOMSG rather than return another thread's
 * message.
 *
 * Test steps:
 * - Receiver gets with rx_source_thread set to an unrelated thread, K_NO_WAIT.
 *
 * Expected result:
 * - k_mbox_get() returns -ENOMSG.
 *
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-13
 * @verifies ZEP-SRS-25-17
 */
ZTEST(mbox_api, test_mbox_incorrect_receiver_tid)
{
	info_type = INCORRECT_RECEIVER_TID;
	tmbox(&mbox);
}

/**
 * @brief Verify a put to a non-waiting target returns -ENOMSG.
 *
 * @details
 * A non-blocking put addressed to a specific target thread that is not waiting
 * to receive must fail with -ENOMSG rather than queue indefinitely.
 *
 * Test steps:
 * - Sender puts with tx_target_thread set to a non-waiting thread, K_NO_WAIT.
 *
 * Expected result:
 * - k_mbox_put() returns -ENOMSG.
 *
 * @see k_mbox_put()
 * @verifies ZEP-SRS-25-13
 * @verifies ZEP-SRS-25-17
 */
ZTEST(mbox_api, test_mbox_incorrect_transmit_tid)
{
	info_type = INCORRECT_TRANSMIT_TID;
	tmbox(&mbox);
}

/**
 * @brief Verify a get with no matching message times out with -EAGAIN.
 *
 * @details
 * A get that filters on a source thread with no pending message and a finite
 * timeout must block for the timeout and then return -EAGAIN.
 *
 * Test steps:
 * - Receiver gets with a non-matching source thread and a finite timeout.
 *
 * Expected result:
 * - k_mbox_get() returns -EAGAIN after the timeout.
 *
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-12
 */
ZTEST(mbox_api, test_mbox_timed_out_mbox_get)
{
	info_type = TIMED_OUT_MBOX_GET;
	tmbox(&mbox);
}

/* Receiver that performs a single blocking empty get, used to satisfy a
 * synchronous put within its timeout.
 */
static void put_timeout_receiver(void *p1, void *p2, void *p3)
{
	struct k_mbox_msg mmsg = {0};

	mmsg.size = 0;
	mmsg.rx_source_thread = K_ANY;
	k_mbox_get((struct k_mbox *)p1, &mmsg, NULL, K_FOREVER);
}

/**
 * @brief Verify synchronous k_mbox_put() honors its timeout.
 *
 * @details
 * A synchronous put blocks until a receiver takes the message or the timeout
 * elapses. With no receiver present a finite-timeout put must fail with -EAGAIN;
 * once a receiver is waiting the same put must succeed.
 *
 * Test steps:
 * - With no receiver, put a message with a finite timeout; expect -EAGAIN.
 * - Start a receiver thread, then put with a finite timeout; expect success.
 *
 * Expected result:
 * - The put without a receiver returns -EAGAIN; with a receiver it returns 0.
 *
 * @see k_mbox_put()
 */
ZTEST(mbox_api, test_mbox_put_timeout)
{
	struct k_mbox_msg mmsg = {0};
	k_tid_t rtid;

	/* No receiver waiting: a finite-timeout put must time out. */
	mmsg.info = PUT_GET_NULL;
	mmsg.size = 0;
	mmsg.tx_data = NULL;
	mmsg.tx_target_thread = K_ANY;
	zassert_equal(k_mbox_put(&mbox, &mmsg, TIMEOUT), -EAGAIN,
		      "put with no receiver did not time out");

	/* Receiver waiting: the same finite-timeout put must succeed. */
	rtid = k_thread_create(&tdata, tstack, STACK_SIZE,
			       put_timeout_receiver, &mbox, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	mmsg.size = 0;
	mmsg.tx_data = NULL;
	mmsg.tx_target_thread = K_ANY;
	zassert_equal(k_mbox_put(&mbox, &mmsg, TIMEOUT), 0,
		      "put with a waiting receiver failed");
	k_thread_abort(rtid);
}

/**
 * @brief Verify a message is not delivered to a get with a mismatched tid.
 *
 * @details
 * A message addressed to one thread must not satisfy a get filtering on a
 * different source thread: the targeted message stays queued (the put times
 * out) and the mismatched non-blocking get returns -ENOMSG.
 *
 * Test steps:
 * - Sender puts a message targeted at the sender thread with a finite timeout.
 * - Receiver gets filtering on an unrelated source with K_NO_WAIT.
 *
 * Expected result:
 * - The get returns -ENOMSG; the mismatched message is not delivered.
 *
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-13
 * @verifies ZEP-SRS-25-17
 */
ZTEST(mbox_api, test_mbox_msg_tid_mismatch)
{
	info_type = MSG_TID_MISMATCH;
	tmbox(&mbox);
}

/**
 * @brief Verify a received message can be disposed by requesting size 0.
 *
 * @details
 * A receiver may discard a message's data by performing a get with a receive
 * size of 0, which completes the transfer without copying the payload.
 *
 * Test steps:
 * - Sender puts a buffer message.
 * - Receiver gets it with size 0 to dispose of the data.
 *
 * Expected result:
 * - The get succeeds (returns 0) and the message is consumed without copying.
 *
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-3
 */
ZTEST(mbox_api, test_mbox_dispose_size_0_msg)
{
	info_type = DISPOSE_SIZE_0_MSG;
	tmbox(&mbox);
}

/**
 * @brief Verify an async put completes a get that is already waiting.
 *
 * @details
 * When a receiver is already blocked in k_mbox_get(), a later
 * k_mbox_async_put() must wake it and deliver the message. A helper thread
 * releases the semaphore that gates the async put so it happens after the get
 * is pending.
 *
 * Test steps:
 * - Receiver blocks in k_mbox_get() (K_FOREVER) for a K_ANY source.
 * - A helper releases the semaphore so the sender performs an async put.
 *
 * Expected result:
 * - The waiting get completes successfully once the async put runs.
 *
 * @see k_mbox_async_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-7
 * @verifies ZEP-SRS-25-11
 */
ZTEST(mbox_api, test_mbox_async_put_to_waiting_get)
{
	info_type = ASYNC_PUT_TO_WAITING_GET;
	tmbox(&mbox);
}

/**
 * @brief Verify a waiting get is not satisfied by an async put to another tid.
 *
 * @details
 * A get blocked on a specific source thread must not be completed by an async
 * put addressed to a different thread: the get times out with -EAGAIN, and a
 * follow-up get for K_ANY then drains the stranded message.
 *
 * Test steps:
 * - Receiver waits in k_mbox_get() filtering on a specific source with a finite
 *   timeout while an async put targets a different (random) thread.
 * - Verify the get returns -EAGAIN, then clean up with a K_ANY get.
 *
 * Expected result:
 * - The filtered get times out (-EAGAIN); the message is retrievable via K_ANY.
 *
 * @see k_mbox_async_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-13
 * @verifies ZEP-SRS-25-17
 */
ZTEST(mbox_api, test_mbox_get_waiting_put_incorrect_tid)
{
	info_type = GET_WAITING_PUT_INCORRECT_TID;
	tmbox(&mbox);
}

/**
 * @brief Verify multiple queued async puts are all delivered.
 *
 * @details
 * Several k_mbox_async_put() calls with different target threads queue multiple
 * messages; the receiver must be able to retrieve them, including selecting a
 * message by matching source thread.
 *
 * Test steps:
 * - Issue multiple async puts with varying target threads (K_ANY and specific).
 * - Receiver gets the messages, including one filtered by source thread.
 *
 * Expected result:
 * - All queued messages are retrieved successfully.
 *
 * @see k_mbox_async_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-7
 */
ZTEST(mbox_api, test_mbox_async_multiple_put)
{
	info_type = ASYNC_MULTIPLE_PUT;
	tmbox(&mbox);
}

/**
 * @brief Verify messages are routed among multiple waiting receivers by tid.
 *
 * @details
 * With several receivers blocked on the same mailbox using different source
 * filters (K_ANY and specific threads), each put message must be delivered to a
 * compatible waiting receiver, exercising tid-based routing under contention.
 *
 * Test steps:
 * - Start five threads that each wait in k_mbox_get() with different source
 *   filters.
 * - Put several messages targeted at K_ANY and specific threads.
 *
 * Expected result:
 * - Every waiting receiver obtains a message matching its source filter.
 *
 * @see k_mbox_put()
 * @see k_mbox_get()
 * @verifies ZEP-SRS-25-9
 * @verifies ZEP-SRS-25-11
 * @verifies ZEP-SRS-25-14
 * @verifies ZEP-SRS-25-15
 */
ZTEST(mbox_api, test_mbox_multiple_waiting_get)
{
	info_type = MULTIPLE_WAITING_GET;
	tmbox(&mbox);

	/* cleanup the sender threads */
	for (int i = 0; i < 5 ; i++) {
		k_thread_abort(&waiting_get_tid[i]);
	}
}

/**
 * @}
 */

/* Suite setup: create the semaphores and the shared mailbox used by the tests. */
void *setup_mbox_api(void)
{
	k_sem_init(&end_sema, 0, 1);
	k_sem_init(&sync_sema, 0, 1);
	k_mbox_init(&mbox);

	return NULL;
}

ZTEST_SUITE(mbox_api, NULL, setup_mbox_api, NULL, NULL, NULL);
