/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_alert_api
 * @{
 * @defgroup t_alert_context test_alert_send_recv_context
 * @brief TestPurpose: verify zephyr alert send/recv across different contexts
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT 100
#define STACK_SIZE 512
#define PENDING_MAX 2
static int alert_handler0(struct k_alert *);
static int alert_handler1(struct k_alert *);

/**TESTPOINT: init via K_ALERT_DEFINE*/
K_ALERT_DEFINE(kalert_pending, alert_handler1, PENDING_MAX);
K_ALERT_DEFINE(kalert_consumed, alert_handler0, PENDING_MAX);

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;
static struct k_alert *palert;
static volatile int handler_executed;

/*handlers*/
static int alert_handler0(struct k_alert *alt)
{
	handler_executed++;
	return 0;
}

static int alert_handler1(struct k_alert *alt)
{
	handler_executed++;
	return 1;
}

static void alert_send(void)
{
	/**TESTPOINT: alert send*/
	for (int i = 0; i < PENDING_MAX; i++) {
		k_alert_send(palert);
	}
}

static void alert_recv(void)
{
	int ret;

	if (palert->handler == K_ALERT_IGNORE ||
			palert->handler == alert_handler0){
		if (palert->handler == alert_handler0)
			zassert_equal(handler_executed, PENDING_MAX, NULL);
		ret = k_alert_recv(palert, TIMEOUT);
		zassert_equal(ret, -EAGAIN, NULL);
	}

	if (palert->handler == K_ALERT_DEFAULT ||
			palert->handler == alert_handler1){
		if (palert->handler == alert_handler1)
			zassert_equal(handler_executed, PENDING_MAX, NULL);
		for (int i = 0; i < PENDING_MAX; i++) {
			/**TESTPOINT: alert recv*/
			ret = k_alert_recv(palert, K_NO_WAIT);
			zassert_false(ret, NULL);
		}
		/**TESTPOINT: alert recv -EAGAIN*/
		ret = k_alert_recv(palert, TIMEOUT);
		zassert_equal(ret, -EAGAIN, NULL);
		/**TESTPOINT: alert recv -EBUSY*/
		ret = k_alert_recv(palert, K_NO_WAIT);
		zassert_equal(ret, -EBUSY, NULL);
	}
}

static void tThread_entry(void *p1, void *p2, void *p3)
{
	alert_recv();
}

static void thread_alert(void)
{
	handler_executed = 0;
	/**TESTPOINT: thread-thread sync via alert*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
		tThread_entry, NULL, NULL, NULL,
		K_PRIO_PREEMPT(0), 0, 0);
	alert_send();
	k_sleep(TIMEOUT);
	k_thread_abort(tid);
}

static void tIsr_entry(void *p)
{
	alert_send();
}

static void isr_alert(void)
{
	handler_executed = 0;
	/**TESTPOINT: thread-isr sync via alert*/
	irq_offload(tIsr_entry, NULL);
	k_sleep(TIMEOUT);
	alert_recv();
}

/*test cases*/
void test_thread_alert_default(void)
{
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_DEFAULT, PENDING_MAX);

	/**TESTPOINT: alert handler default*/
	palert = &alert;
	thread_alert();

}

void test_thread_alert_ignore(void)
{
	/**TESTPOINT: alert handler ignore*/
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_IGNORE, PENDING_MAX);
	palert = &alert;
	thread_alert();
}

void test_thread_alert_consumed(void)
{
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, alert_handler0, PENDING_MAX);

	/**TESTPOINT: alert handler return 0*/
	palert = &alert;
	thread_alert();
}

void test_thread_alert_pending(void)
{
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, alert_handler1, PENDING_MAX);

	/**TESTPOINT: alert handler return 1*/
	palert = &alert;
	thread_alert();
}

void test_isr_alert_default(void)
{
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_DEFAULT, PENDING_MAX);

	/**TESTPOINT: alert handler default*/
	palert = &alert;
	isr_alert();
}

void test_isr_alert_ignore(void)
{
	/**TESTPOINT: alert handler ignore*/
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_IGNORE, PENDING_MAX);
	palert = &alert;
	isr_alert();
}

void test_isr_alert_consumed(void)
{
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, alert_handler0, PENDING_MAX);

	/**TESTPOINT: alert handler return 0*/
	palert = &alert;
	isr_alert();
}

void test_isr_alert_pending(void)
{
	struct k_alert alert;
	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, alert_handler1, PENDING_MAX);

	/**TESTPOINT: alert handler return 0*/
	palert = &alert;
	isr_alert();
}

void test_thread_kinit_alert(void)
{
	palert = &kalert_consumed;
	thread_alert();
	palert = &kalert_pending;
	thread_alert();
}

void test_isr_kinit_alert(void)
{
	palert = &kalert_consumed;
	isr_alert();
	palert = &kalert_pending;
	isr_alert();
}
