/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_mbox
 * @{
 * @defgroup t_mbox_api test_mbox_api
 * @brief TestPurpose: verify data passing via mailbox APIs
 * - API coverage
 *   -# K_MBOX_DEFINE
 *   -# k_mbox_init
 *   -# k_mbox_put
 *   -# k_mbox_async_put
 *   -# k_mbox_get
 *   -# k_mbox_data_get
 *   -# k_mbox_data_block_get
 * @}
 */

#include <ztest.h>

#define TIMEOUT 100
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define MAIL_LEN 64


/**TESTPOINT: init via K_MBOX_DEFINE*/
K_MBOX_DEFINE(kmbox);
K_MEM_POOL_DEFINE(mpooltx, 8, MAIL_LEN, 1, 4);
K_MEM_POOL_DEFINE(mpoolrx, 8, MAIL_LEN, 1, 4);

static struct k_mbox mbox;

static k_tid_t sender_tid, receiver_tid;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

static struct k_sem end_sema, sync_sema;

static enum mmsg_type {
	PUT_GET_NULL = 0,
	PUT_GET_BUFFER,
	ASYNC_PUT_GET_BUFFER,
	ASYNC_PUT_GET_BLOCK,
	TARGET_SOURCE_THREAD_BUFFER,
	TARGET_SOURCE_THREAD_BLOCK,
	MAX_INFO_TYPE
} info_type;

static char data[MAX_INFO_TYPE][MAIL_LEN] = {
	"send/recv an empty message",
	"send/recv msg using a buffer",
	"async send/recv msg using a buffer",
	"async send/recv msg using a memory block",
	"specify target/source thread, using a buffer",
	"specify target/source thread, using a memory block"
};

static void tmbox_put(struct k_mbox *pmbox)
{
	struct k_mbox_msg mmsg;

	memset(&mmsg, 0, sizeof(mmsg));

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
		/*fall through*/
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
		/*fall through*/
	case TARGET_SOURCE_THREAD_BLOCK:
		/**TESTPOINT: mbox async put mem block*/
		mmsg.info = ASYNC_PUT_GET_BLOCK;
		mmsg.size = MAIL_LEN;
		mmsg.tx_data = NULL;
		zassert_equal(k_mem_pool_alloc(&mpooltx, &mmsg.tx_block,
			MAIL_LEN, K_NO_WAIT), 0, NULL);
		memcpy(mmsg.tx_block.data, data[info_type], MAIL_LEN);
		if (info_type == TARGET_SOURCE_THREAD_BLOCK) {
			mmsg.tx_target_thread = receiver_tid;
		} else {
			mmsg.tx_target_thread = K_ANY;
		}
		k_mbox_async_put(pmbox, &mmsg, &sync_sema);
		/*wait for msg being taken*/
		k_sem_take(&sync_sema, K_FOREVER);
		break;
	default:
		break;
	}
}

static void tmbox_get(struct k_mbox *pmbox)
{
	struct k_mbox_msg mmsg;
	char rxdata[MAIL_LEN];
	struct k_mem_block rxblock;

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
		/*fall through*/
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
		/*fall through*/
	case TARGET_SOURCE_THREAD_BLOCK:
		/**TESTPOINT: mbox async get mem block*/
		mmsg.size = MAIL_LEN;
		if (info_type == TARGET_SOURCE_THREAD_BLOCK) {
			mmsg.rx_source_thread = sender_tid;
		} else {
			mmsg.rx_source_thread = K_ANY;
		}
		zassert_true(k_mbox_get(pmbox, &mmsg, NULL, K_FOREVER) == 0,
			NULL);
		zassert_true(k_mbox_data_block_get
			(&mmsg, &mpoolrx, &rxblock, K_FOREVER) == 0, NULL);
		zassert_equal(mmsg.info, ASYNC_PUT_GET_BLOCK, NULL);
		zassert_equal(mmsg.size, MAIL_LEN, NULL);
		/*verify rxblock*/
		zassert_true(memcmp(rxblock.data, data[info_type], MAIL_LEN)
			== 0, NULL);
		k_mem_pool_free(&rxblock);
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
		K_PRIO_PREEMPT(0), 0, 0);
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

void test_mbox_target_source_thread_block(void)
{
	info_type = TARGET_SOURCE_THREAD_BLOCK;
	tmbox(&mbox);
}
