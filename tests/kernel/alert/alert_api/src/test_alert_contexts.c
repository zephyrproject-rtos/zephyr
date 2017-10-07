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

enum handle_type {
	HANDLER_IGNORE,
	HANDLER_DEFAULT,
	HANDLER_0,
	HANDLER_1
};

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
__kernel struct k_thread tdata;
__kernel struct k_alert thread_alerts[4];
static struct k_alert *palert;
static enum handle_type htype;
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

	switch (htype) {
	case HANDLER_0:
		zassert_equal(handler_executed, PENDING_MAX, NULL);
		/* Fall through */
	case HANDLER_IGNORE:
		ret = k_alert_recv(palert, TIMEOUT);
		zassert_equal(ret, -EAGAIN, NULL);
		break;
	case HANDLER_1:
		zassert_equal(handler_executed, PENDING_MAX, NULL);
		/* Fall through */
	case HANDLER_DEFAULT:
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

static void thread_entry(void *p1, void *p2, void *p3)
{
	alert_recv();
}

static void thread_alert(void)
{
	handler_executed = 0;
	/**TESTPOINT: thread-thread sync via alert*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(0),
				      K_USER | K_INHERIT_PERMS,
				      0);
	alert_send();
	k_sleep(TIMEOUT);
	k_thread_abort(tid);
}

static void tisr_entry(void *p)
{
	alert_send();
}

static void isr_alert(void)
{
	handler_executed = 0;
	/**TESTPOINT: thread-isr sync via alert*/
	irq_offload(tisr_entry, NULL);
	k_sleep(TIMEOUT);
	alert_recv();
}

/*test cases*/
void test_thread_alert_default(void)
{
	palert = &thread_alerts[HANDLER_DEFAULT];
	htype = HANDLER_DEFAULT;
	thread_alert();
}

void test_thread_alert_ignore(void)
{
	palert = &thread_alerts[HANDLER_IGNORE];
	htype = HANDLER_IGNORE;
	thread_alert();
}

void test_thread_alert_consumed(void)
{
	/**TESTPOINT: alert handler return 0*/
	palert = &thread_alerts[HANDLER_0];
	htype = HANDLER_0;
	thread_alert();
}

void test_thread_alert_pending(void)
{
	/**TESTPOINT: alert handler return 1*/
	palert = &thread_alerts[HANDLER_1];
	htype = HANDLER_1;
	thread_alert();
}

void test_isr_alert_default(void)
{
	struct k_alert alert;

	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_DEFAULT, PENDING_MAX);

	/**TESTPOINT: alert handler default*/
	palert = &alert;
	htype = HANDLER_DEFAULT;
	isr_alert();
}

void test_isr_alert_ignore(void)
{
	/**TESTPOINT: alert handler ignore*/
	struct k_alert alert;

	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_IGNORE, PENDING_MAX);
	palert = &alert;
	htype = HANDLER_IGNORE;
	isr_alert();
}

void test_isr_alert_consumed(void)
{
	struct k_alert alert;

	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, alert_handler0, PENDING_MAX);

	/**TESTPOINT: alert handler return 0*/
	palert = &alert;
	htype = HANDLER_0;
	isr_alert();
}

void test_isr_alert_pending(void)
{
	struct k_alert alert;

	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, alert_handler1, PENDING_MAX);

	/**TESTPOINT: alert handler return 0*/
	palert = &alert;
	htype = HANDLER_1;
	isr_alert();
}

void test_thread_kinit_alert(void)
{
	palert = &kalert_consumed;
	htype = HANDLER_0;
	thread_alert();
	palert = &kalert_pending;
	htype = HANDLER_1;
	thread_alert();
}

void test_isr_kinit_alert(void)
{
	palert = &kalert_consumed;
	htype = HANDLER_0;
	isr_alert();
	palert = &kalert_pending;
	htype = HANDLER_1;
	isr_alert();
}

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &kalert_pending,
			      &kalert_consumed, &tdata, &tstack,
			      &thread_alerts[HANDLER_DEFAULT],
			      &thread_alerts[HANDLER_IGNORE],
			      &thread_alerts[HANDLER_0],
			      &thread_alerts[HANDLER_1], NULL);

	k_alert_init(&thread_alerts[HANDLER_DEFAULT], K_ALERT_DEFAULT,
		     PENDING_MAX);
	k_alert_init(&thread_alerts[HANDLER_IGNORE], K_ALERT_IGNORE,
		     PENDING_MAX);
	k_alert_init(&thread_alerts[HANDLER_0], alert_handler0, PENDING_MAX);
	k_alert_init(&thread_alerts[HANDLER_1], alert_handler1, PENDING_MAX);

	ztest_test_suite(test_alert_api,
			 ztest_user_unit_test(test_thread_alert_default),
			 ztest_user_unit_test(test_thread_alert_ignore),
			 ztest_user_unit_test(test_thread_alert_consumed),
			 ztest_user_unit_test(test_thread_alert_pending),
			 ztest_unit_test(test_isr_alert_default),
			 ztest_unit_test(test_isr_alert_ignore),
			 ztest_unit_test(test_isr_alert_consumed),
			 ztest_unit_test(test_isr_alert_pending),
			 ztest_user_unit_test(test_thread_kinit_alert),
			 ztest_unit_test(test_isr_kinit_alert));
	ztest_run_test_suite(test_alert_api);
}
