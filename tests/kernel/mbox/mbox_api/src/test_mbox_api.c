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

/**
 * @brief Tests for the mailbox kernel object
 * @defgroup kernel_mbox_api Mailbox
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
		__fallthrough;
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
		/* timeout because this msg wont be received with a _get*/
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
		__fallthrough;
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

/*test cases*/
ZTEST(mbox_api, test_mbox_kinit)
{
	/**TESTPOINT: init via k_mbox_init*/
	k_mbox_init(&mbox);
}

ZTEST(mbox_api, test_mbox_kdefine)
{
	info_type = PUT_GET_NULL;
	tmbox(&kmbox);
}

static ZTEST_BMEM char __aligned(4) buffer[8];

/**
 *
 * @brief Test mailbox enhance capabilities
 *
 * @details
 * - Define and initialized a message queue and a mailbox
 * - Verify the capability of message queue and mailbox
 * - with same data.
 *
 * @ingroup kernel_mbox_api
 *
 * @see k_msgq_init() k_msgq_put() k_mbox_async_put() k_mbox_get()
 */
ZTEST(mbox_api, test_enhance_capability)
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

/*
 *
 * @brife Test any number of mailbox can be defined
 *
 * @details
 * - Define multi mailbox and verify the mailbox whether as
 *   expected
 * - Verify the mailbox can be used
 *
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

ZTEST(mbox_api, test_mbox_put_get_null)
{
	info_type = PUT_GET_NULL;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_put_get_buffer)
{
	info_type = PUT_GET_BUFFER;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_async_put_get_buffer)
{
	info_type = ASYNC_PUT_GET_BUFFER;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_async_put_get_block)
{
	info_type = ASYNC_PUT_GET_BLOCK;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_target_source_thread_buffer)
{
	info_type = TARGET_SOURCE_THREAD_BUFFER;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_incorrect_receiver_tid)
{
	info_type = INCORRECT_RECEIVER_TID;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_incorrect_transmit_tid)
{
	info_type = INCORRECT_TRANSMIT_TID;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_timed_out_mbox_get)
{
	info_type = TIMED_OUT_MBOX_GET;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_msg_tid_mismatch)
{
	info_type = MSG_TID_MISMATCH;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_dispose_size_0_msg)
{
	info_type = DISPOSE_SIZE_0_MSG;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_async_put_to_waiting_get)
{
	info_type = ASYNC_PUT_TO_WAITING_GET;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_get_waiting_put_incorrect_tid)
{
	info_type = GET_WAITING_PUT_INCORRECT_TID;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_async_multiple_put)
{
	info_type = ASYNC_MULTIPLE_PUT;
	tmbox(&mbox);
}

ZTEST(mbox_api, test_mbox_multiple_waiting_get)
{
	info_type = MULTIPLE_WAITING_GET;
	tmbox(&mbox);

	/* cleanup the sender threads */
	for (int i = 0; i < 5 ; i++) {
		k_thread_abort(&waiting_get_tid[i]);
	}
}

void *setup_mbox_api(void)
{
	k_sem_init(&end_sema, 0, 1);
	k_sem_init(&sync_sema, 0, 1);
	k_mbox_init(&mbox);

	return NULL;
}

ZTEST_SUITE(mbox_api, NULL, setup_mbox_api, NULL, NULL, NULL);
