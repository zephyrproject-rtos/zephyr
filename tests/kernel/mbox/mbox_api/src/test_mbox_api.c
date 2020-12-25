/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#define TIMEOUT K_MSEC(100)
#if !defined(CONFIG_BOARD_QEMU_X86)
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#else
#define STACK_SIZE (640 + CONFIG_TEST_EXTRA_STACKSIZE)
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
static K_THREAD_STACK_DEFINE(rtstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack_1, STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(waiting_get_stack, 5, STACK_SIZE);
static struct k_thread tdata, rtdata, async_tid, waiting_get_tid[5];

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
		mmsg.tx_block.data = NULL;
		mmsg.tx_target_thread = K_ANY;
		zassert_true(k_mbox_put(pmbox, &mmsg, K_FOREVER) == 0, NULL);
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
		zassert_equal(mmsg.info, PUT_GET_NULL, NULL);
		/*verify .size*/
		zassert_equal(mmsg.size, 0, NULL);
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
		zassert_equal(mmsg.info, PUT_GET_BUFFER, NULL);
		zassert_equal(mmsg.size, sizeof(data[info_type]), NULL);
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
		zassert_equal(mmsg.info, ASYNC_PUT_GET_BUFFER, NULL);
		zassert_equal(mmsg.size, sizeof(data[info_type]), NULL);
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
void test_mbox_kinit(void)
{
	/**TESTPOINT: init via k_mbox_init*/
	k_mbox_init(&mbox);
	k_sem_init(&end_sema, 0, 1);
	k_sem_init(&sync_sema, 0, 1);
}

void test_mbox_kdefine(void)
{
	info_type = PUT_GET_NULL;
	tmbox(&kmbox);
}

K_MBOX_DEFINE(send_mbox);
static void thread_mbox_data_get_null(void *p1, void *p2, void *p3)
{
	struct k_mbox_msg get_msg = {0};
	char str_data[] = "it string for get msg test";

	get_msg.size = 16;
	get_msg.rx_source_thread = K_ANY;
	get_msg.tx_block.data = str_data;
	get_msg._syncing_thread = receiver_tid;

	k_mbox_data_get(&get_msg, NULL);

	get_msg._syncing_thread = NULL;
	k_mbox_data_get(&get_msg, NULL);
	k_sem_give(&end_sema);
}

/**
 *
 * @brief Test k_mbox_data_get() API
 *
 * @details
 * - Init a mbox and just invoke k_mbox_data_get() with different
 *   input to check robust of API.
 *
 * @see k_mbox_data_get()
 *
 * @ingroup kernel_mbox_api
 */
void test_mbox_data_get_null(void)
{
	k_sem_reset(&end_sema);

	receiver_tid = k_thread_create(&tdata, tstack, STACK_SIZE,
					thread_mbox_data_get_null,
					NULL, NULL, NULL,
					K_PRIO_PREEMPT(0), 0,
					K_NO_WAIT);
	k_sem_take(&end_sema, K_FOREVER);
	/*test case teardown*/
	k_thread_abort(receiver_tid);
}

static void thread_mbox_get_block_data(void *p1, void *p2, void *p3)
{
	k_sem_take(&sync_sema, K_FOREVER);
	struct k_mbox_msg bdmsg;

	bdmsg.size = MAIL_LEN;
	bdmsg.rx_source_thread = sender_tid;
	bdmsg.tx_target_thread = receiver_tid;
	bdmsg.tx_data = NULL;
	bdmsg.tx_block.data = data;
	bdmsg.tx_data = data;

	zassert_equal(k_mbox_get((struct k_mbox *)p1, &bdmsg, p2, K_FOREVER),
			0, NULL);

	k_sem_give(&end_sema);
}

/* give a block data to API k_mbox_async_put */
static void thread_mbox_put_block_data(void *p1, void *p2, void *p3)
{
	struct k_mbox_msg put_msg;

	put_msg.size = MAIL_LEN;
	put_msg.tx_data = NULL;
	put_msg.tx_block.data = p2;
	put_msg.tx_target_thread = receiver_tid;
	put_msg.rx_source_thread = sender_tid;

	k_mbox_async_put((struct k_mbox *)p1, &put_msg, NULL);
}

/**
 *
 * @brief Test put and get mailbox with block data
 *
 * @details
 * - Create two threads to put and get block data with
 *   specify thread ID and K_FOREVER for each other.
 * - Check the result after finished exchange.
 *
 * @see k_mbox_init() k_mbox_async_put() k_mbox_get()
 *
 * @ingroup kernel_mbox_api
 */
void test_mbox_get_put_block_data(void)
{
	struct k_mbox bdmbox;
	/*test case setup*/
	k_sem_reset(&end_sema);
	k_sem_reset(&sync_sema);
	k_mbox_init(&bdmbox);
	char buff[MAIL_LEN];
	char data_put[] = "mbox put data";

	/**TESTPOINT: thread-thread data passing via mbox*/
	sender_tid = k_current_get();
	receiver_tid = k_thread_create(&rtdata, rtstack, STACK_SIZE,
				       thread_mbox_get_block_data,
				       &bdmbox, buff, NULL,
				       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	sender_tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_mbox_put_block_data,
				      &bdmbox, data_put, NULL,
				       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	k_sem_give(&sync_sema);
	k_sem_take(&end_sema, K_FOREVER);
	/*abort receiver thread*/
	k_thread_abort(receiver_tid);
	k_thread_abort(sender_tid);

	zassert_equal(memcmp(buff, data_put, sizeof(data_put)), 0,
			     NULL);
}

/**
 *
 * @brief Test mailbox enhance capabilities
 *
 * @details
 * - Define and initilized a message queue and a mailbox
 * - Verify the capability of message queue and mailbox
 * - with same data.
 *
 * @ingroup kernel_mbox_api
 *
 * @see k_msgq_init() k_msgq_put() k_mbox_async_put() k_mbox_get()
 */
static ZTEST_BMEM char __aligned(4) buffer[8];
void test_enhance_capability(void)
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
void test_define_multi_mbox(void)
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

void test_mbox_put_get_null(void)
{
	info_type = PUT_GET_NULL;
	tmbox(&mbox);
}

void test_mbox_put_get_buffer(void)
{
	info_type = PUT_GET_BUFFER;
	tmbox(&mbox);
}

void test_mbox_async_put_get_buffer(void)
{
	info_type = ASYNC_PUT_GET_BUFFER;
	tmbox(&mbox);
}

void test_mbox_async_put_get_block(void)
{
	info_type = ASYNC_PUT_GET_BLOCK;
	tmbox(&mbox);
}

void test_mbox_target_source_thread_buffer(void)
{
	info_type = TARGET_SOURCE_THREAD_BUFFER;
	tmbox(&mbox);
}

void test_mbox_incorrect_receiver_tid(void)
{
	info_type = INCORRECT_RECEIVER_TID;
	tmbox(&mbox);
}

void test_mbox_incorrect_transmit_tid(void)
{
	info_type = INCORRECT_TRANSMIT_TID;
	tmbox(&mbox);
}

void test_mbox_timed_out_mbox_get(void)
{
	info_type = TIMED_OUT_MBOX_GET;
	tmbox(&mbox);
}

void test_mbox_msg_tid_mismatch(void)
{
	info_type = MSG_TID_MISMATCH;
	tmbox(&mbox);
}

void test_mbox_dispose_size_0_msg(void)
{
	info_type = DISPOSE_SIZE_0_MSG;
	tmbox(&mbox);
}

void test_mbox_async_put_to_waiting_get(void)
{
	info_type = ASYNC_PUT_TO_WAITING_GET;
	tmbox(&mbox);
}

void test_mbox_get_waiting_put_incorrect_tid(void)
{
	info_type = GET_WAITING_PUT_INCORRECT_TID;
	tmbox(&mbox);
}

void test_mbox_async_multiple_put(void)
{
	info_type = ASYNC_MULTIPLE_PUT;
	tmbox(&mbox);
}

void test_mbox_multiple_waiting_get(void)
{
	info_type = MULTIPLE_WAITING_GET;
	tmbox(&mbox);
}
