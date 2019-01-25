/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define MAIL_LEN 64

K_MEM_POOL_DEFINE(mpooltx, 8, MAIL_LEN, 1, 4);
K_MEM_POOL_DEFINE(mpoolrx, 8, MAIL_LEN, 1, 4);

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);

static struct k_thread tdata;

static struct k_mbox mbox;

static struct k_sem sync_sema;

static k_tid_t tid1, receiver_tid;

static enum mmsg_type {
	PUT_GET_NULL = 0,
	TARGET_SOURCE
} info_type;

static void msg_sender(struct k_mbox *pmbox, s32_t timeout)
{
	struct k_mbox_msg mmsg;

	(void)memset(&mmsg, 0, sizeof(mmsg));

	switch (info_type) {
	case PUT_GET_NULL:
		/* mbox sync put empty message */
		mmsg.info = PUT_GET_NULL;
		mmsg.size = 0;
		mmsg.tx_data = NULL;
		if (timeout == K_FOREVER) {
			k_mbox_put(pmbox, &mmsg, K_FOREVER);
		} else if (timeout == K_NO_WAIT) {
			k_mbox_put(pmbox, &mmsg, K_NO_WAIT);
		} else {
			k_mbox_put(pmbox, &mmsg, timeout);
		}
		break;
	default:
		break;
	}
}

static void msg_receiver(struct k_mbox *pmbox, k_tid_t thd_id, s32_t timeout)
{
	struct k_mbox_msg mmsg;
	char rxdata[MAIL_LEN];

	switch (info_type) {
	case PUT_GET_NULL:
		mmsg.size = sizeof(rxdata);
		mmsg.rx_source_thread = thd_id;
		if (timeout == K_FOREVER) {
			zassert_true(k_mbox_get(pmbox, &mmsg,
				     rxdata, K_FOREVER) == 0, NULL);
		} else if (timeout == K_NO_WAIT) {
			zassert_false(k_mbox_get(pmbox, &mmsg,
				      rxdata, K_NO_WAIT) == 0, NULL);
		} else {
			zassert_true(k_mbox_get(pmbox, &mmsg,
				     rxdata, timeout) == 0, NULL);
		}
		break;
	default:
		break;
	}
}

static void test_mbox_init(void)
{
	k_mbox_init(&mbox);

	k_sem_init(&sync_sema, 0, 1);
}

void test_send(void *p1, void *p2, void *p3)
{
	msg_sender((struct k_mbox *)p1, K_NO_WAIT);
}

/* Receive message from any thread with no wait */
void test_msg_receiver(void)
{
	static k_tid_t tid;

	info_type = PUT_GET_NULL;
	msg_receiver(&mbox, K_ANY, K_NO_WAIT);

	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      test_send, &mbox, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	msg_receiver(&mbox, K_ANY, 2);
	k_thread_abort(tid);
}

void test_send_un(void *p1, void *p2, void *p3)
{
	TC_PRINT("Sender UNLIMITED\n");
	msg_sender((struct k_mbox *)p1, K_FOREVER);
}

/* Receive message from thread tid1 */
void msg_receiver_unlimited(void)
{
	info_type = PUT_GET_NULL;

	receiver_tid = k_current_get();
	tid1 = k_thread_create(&tdata, tstack, STACK_SIZE,
			      test_send_un, &mbox, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	msg_receiver(&mbox, tid1, K_FOREVER);
	k_thread_abort(tid1);
}

/*test case main entry*/
void test_main(void)
{
	test_mbox_init();
	ztest_test_suite(test_mbox,
			 ztest_unit_test(test_msg_receiver),
			 ztest_unit_test(msg_receiver_unlimited));
	ztest_run_test_suite(test_mbox);
}
