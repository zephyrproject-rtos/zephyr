/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/kernel.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define MAIL_LEN 64
#define HIGH_PRIO 1
#define LOW_PRIO  8

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(high_stack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(low_stack, STACK_SIZE);

static struct k_thread tdata, high_tdata, low_tdata;
static struct k_mbox mbox, multi_tmbox;
static struct k_sem sync_sema;
static k_tid_t tid1, receiver_tid;
static char msg_data[2][MAIL_LEN] = {
	"send to high prio",
	"send to low prio"};

static enum mmsg_type {
	PUT_GET_NULL = 0,
	TARGET_SOURCE
} info_type;

static void msg_sender(struct k_mbox *pmbox, k_timeout_t timeout)
{
	static struct k_mbox_msg mmsg;

	(void)memset(&mmsg, 0, sizeof(mmsg));

	switch (info_type) {
	case PUT_GET_NULL:
		/* mbox sync put empty message */
		mmsg.info = PUT_GET_NULL;
		mmsg.size = 0;
		mmsg.tx_data = NULL;
		if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			k_mbox_put(pmbox, &mmsg, K_FOREVER);
		} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
			k_mbox_put(pmbox, &mmsg, K_NO_WAIT);
		} else {
			k_mbox_put(pmbox, &mmsg, timeout);
		}
		break;
	default:
		break;
	}
}

static void msg_receiver(struct k_mbox *pmbox, k_tid_t thd_id,
			 k_timeout_t timeout)
{
	static struct k_mbox_msg mmsg;
	static char rxdata[MAIL_LEN];

	switch (info_type) {
	case PUT_GET_NULL:
		mmsg.size = sizeof(rxdata);
		mmsg.rx_source_thread = thd_id;
		if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
			zassert_true(k_mbox_get(pmbox, &mmsg,
				     rxdata, K_FOREVER) == 0, NULL);
		} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
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
	k_mbox_init(&multi_tmbox);

	k_sem_init(&sync_sema, 0, 2);
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

	msg_receiver(&mbox, K_ANY, K_MSEC(2));
	k_thread_abort(tid);
}

void test_send_un(void *p1, void *p2, void *p3)
{
	TC_PRINT("Sender UNLIMITED\n");
	msg_sender((struct k_mbox *)p1, K_FOREVER);
}

/* Receive message from thread tid1 */
void test_msg_receiver_unlimited(void)
{
	info_type = PUT_GET_NULL;

	receiver_tid = k_current_get();
	tid1 = k_thread_create(&tdata, tstack, STACK_SIZE,
			      test_send_un, &mbox, NULL, NULL,
			      K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	msg_receiver(&mbox, tid1, K_FOREVER);
	k_thread_abort(tid1);
}

void thread_low_prio(void *p1, void *p2, void *p3)
{
	static struct k_mbox_msg mmsg = {0};
	static char rxdata[MAIL_LEN];
	int ret;

	mmsg.rx_source_thread = K_ANY;
	mmsg.size = sizeof(rxdata);
	ret = k_mbox_get(&multi_tmbox, &mmsg, rxdata, K_FOREVER);

	zassert_equal(ret, 0, "low prio get msg failed");
	zassert_equal(memcmp(rxdata, msg_data[1], MAIL_LEN), 0,
		      "low prio data error");

	k_sem_give(&sync_sema);
}

void thread_high_prio(void *p1, void *p2, void *p3)
{
	static struct k_mbox_msg mmsg = {0};
	static char rxdata[MAIL_LEN];
	int ret;

	mmsg.rx_source_thread = K_ANY;
	mmsg.size = sizeof(rxdata);
	ret = k_mbox_get(&multi_tmbox, &mmsg, rxdata, K_FOREVER);

	zassert_equal(ret, 0, "high prio get msg failed");
	zassert_equal(memcmp(rxdata, msg_data[0], MAIL_LEN), 0,
		      "high prio data error");

	k_sem_give(&sync_sema);
}

void test_multi_thread_send_get(void)
{
	static k_tid_t low_prio, high_prio;
	struct k_mbox_msg mmsg = {0};

	k_sem_reset(&sync_sema);
	/* Create diff priority thread to receive msg with same mbox */
	low_prio = k_thread_create(&low_tdata, low_stack, STACK_SIZE,
				  thread_low_prio, &multi_tmbox, NULL, NULL,
				  LOW_PRIO, 0, K_NO_WAIT);

	high_prio = k_thread_create(&high_tdata, high_stack, STACK_SIZE,
				    thread_high_prio, &multi_tmbox, NULL, NULL,
				    HIGH_PRIO, 0, K_NO_WAIT);

	k_sleep(K_MSEC(20));
	mmsg.size = sizeof(msg_data[0]);
	mmsg.tx_data = msg_data[0];
	mmsg.tx_target_thread = K_ANY;
	k_mbox_put(&multi_tmbox, &mmsg, K_FOREVER);

	mmsg.size = sizeof(msg_data[1]);
	mmsg.tx_data = msg_data[1];
	mmsg.tx_target_thread = K_ANY;
	k_mbox_put(&multi_tmbox, &mmsg, K_FOREVER);

	/* Sync with threads to ensure process end */
	k_sem_take(&sync_sema, K_FOREVER);
	k_sem_take(&sync_sema, K_FOREVER);

	k_thread_abort(low_prio);
	k_thread_abort(high_prio);
}

/*test case main entry*/
void test_main(void)
{
	test_mbox_init();
	ztest_test_suite(test_mbox,
			 ztest_unit_test(test_msg_receiver),
			 ztest_unit_test(test_msg_receiver_unlimited),
			 ztest_1cpu_user_unit_test(test_multi_thread_send_get));
	ztest_run_test_suite(test_mbox);
}
