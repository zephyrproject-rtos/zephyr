/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT 100
#define STACK_SIZE 512
#define PENDING_MAX 2
#define SEM_INITIAL 0
#define SEM_LIMIT 1

K_SEM_DEFINE(sync_sema, SEM_INITIAL, SEM_LIMIT);

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
static K_THREAD_STACK_DEFINE(sync_tstack, STACK_SIZE);
__kernel struct k_thread tdata;
__kernel struct k_thread sync_tdata;
__kernel struct k_alert thread_alerts[4];
static struct k_alert *palert;
static enum handle_type htype;
static volatile u32_t handler_executed;
static volatile u32_t handler_val;


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

static void sync_entry(void *p)
{
	k_alert_send(palert);
}

static void isr_alert(void)
{
	handler_executed = 0;
	/**TESTPOINT: thread-isr sync via alert*/
	irq_offload(tisr_entry, NULL);
	k_sleep(TIMEOUT);
	alert_recv();
}

int event_handler(struct k_alert *alt)
{
	return handler_val;
}

/**
 * @brief Tests for the Alert kernel object
 * @defgroup kernel_alert_tests Alerts
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Test thread alert default
 *
 * Checks k_alert_init(), k_alert_send(), k_alert_recv() Kernel APIs.
 *
 * Initializes an alert and creates a thread that signals an alert with
 * k_alert_send() and then calls k_alert_recv() with K_NO_WAIT to receive the
 * alerts. Checks if k_alert_recv() returns appropriate error values when
 * alerts are not received.
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
void test_thread_alert_default(void)
{
	palert = &thread_alerts[HANDLER_DEFAULT];
	htype = HANDLER_DEFAULT;
	thread_alert();
}

/**
 * @brief Test thread alert ignore
 *
 * Checks k_alert_init(), k_alert_send(), k_alert_recv() Kernel APIs - creates
 * a thread that signals an alert using k_alert_send() and then calls
 * k_alert_recv() with TIMEOUT of 100ms which is the waiting period for
 * receiving the alert.
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
void test_thread_alert_ignore(void)
{
	palert = &thread_alerts[HANDLER_IGNORE];
	htype = HANDLER_IGNORE;
	thread_alert();
}

/**
 * @brief Test thread alert consumed
 *
 * Checks k_alert_init(), k_alert_send(), k_alert_recv() Kernel APIs.
 *
 * Creates a thread that signals an alert using k_alert_send(). Now
 * k_alert_send() for this case is initialized with an address of the handler
 * function, which increases handler_executed count each time it is called.
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
void test_thread_alert_consumed(void)
{
	/**TESTPOINT: alert handler return 0*/
	palert = &thread_alerts[HANDLER_0];
	htype = HANDLER_0;
	thread_alert();
}

/**
 * @brief Test thread alert pending
 *
 * Checks k_alert_init(), k_alert_send(), k_alert_recv() Kernel APIs
 *
 * Creates a thread that signals an alert using k_alert_send().
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
void test_thread_alert_pending(void)
{
	/**TESTPOINT: alert handler return 1*/
	palert = &thread_alerts[HANDLER_1];
	htype = HANDLER_1;
	thread_alert();
}

/**
 * @brief Test default isr alert
 *
 * Similar to test_thread_alert_default(), but verifies kernel objects and
 * APIs work correctly in interrupt context with the help of irq_offload()
 * function.
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
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

/**
 * @brief Test isr alert ignore
 *
 * Similar to test_thread_alert_ignore(), but verifies kernel objects and
 * APIs work correctly in interrupt context with the help of irq_offload()
 * function.
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
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

/**
 * @brief Test isr alert consumed
 *
 * Similar to test_thread_alert_consumed, but verifies kernel objects
 * and APIs work correctly in interrupt context with the help of irq_offload()
 * function.
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
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

/**
 * @brief Test isr alert pending
 *
 * Similar to test_thread_alert_pending(), but verifies kernel objects and
 * APIs work correctly in interrupt context with the help of irq_offload()
 * function.
 *
 * @see k_alert_init(), k_alert_send(), k_alert_recv()
 */
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

/**
 * @brief Test thread kinit alert
 *
 * Tests consumed and pending thread alert cases (reference line numbers 4 and
 * 5), but handles case where alert has been defined via #K_ALERT_DEFINE(x) and not
 * k_alert_init()
 *
 * @see #K_ALERT_DEFINE(x)
 */
void test_thread_kinit_alert(void)
{
	palert = &kalert_consumed;
	htype = HANDLER_0;
	thread_alert();
	palert = &kalert_pending;
	htype = HANDLER_1;
	thread_alert();
}

/**
 * @brief Test isr kinit alert
 *
 * Checks consumed and pending isr alert cases but alert has been defined via
 * #K_ALERT_DEFINE(x) and not k_alert_init()
 *
 * @see #K_ALERT_DEFINE(x)
 */
void test_isr_kinit_alert(void)
{
	palert = &kalert_consumed;
	htype = HANDLER_0;
	isr_alert();
	palert = &kalert_pending;
	htype = HANDLER_1;
	isr_alert();
}


/**
 * @brief Test alert_recv(timeout)
 *
 * This test checks k_alert_recv(timeout) against the following cases:
 *  1. The current task times out while waiting for the event.
 *  2. There is already an event waiting (signaled from a task).
 *  3. The current task must wait on the event until it is signaled
 *     from either another task or an ISR.
 *
 * @see k_alert_recv()
 */
void test_thread_alert_timeout(void)
{
	/**TESTPOINT: alert handler ignore*/
	struct k_alert alert;
	int ret, i;

	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_DEFAULT, PENDING_MAX);

	palert = &alert;

	ret = k_alert_recv(&alert, TIMEOUT);

	zassert_equal(ret, -EAGAIN, NULL);

	k_alert_send(&alert);

	ret = k_alert_recv(&alert, TIMEOUT);

	zassert_equal(ret, 0, NULL);

	k_sem_give(&sync_sema);

	for (i = 0; i < 2; i++) {
		ret = k_alert_recv(&alert, TIMEOUT);

		zassert_equal(ret, 0, NULL);
	}
}

/**
 * @brief Test k_alert_recv() against different cases
 *
 * This test checks k_alert_recv(K_FOREVER) against
 * the following cases:
 *  1. There is already an event waiting (signaled from a task and ISR).
 *  2. The current task must wait on the event until it is signaled
 *     from either another task or an ISR.
 *
 * @see k_alert_recv()
 */
void test_thread_alert_wait(void)
{
	/**TESTPOINT: alert handler ignore*/
	struct k_alert alert;
	int ret, i;

	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, K_ALERT_DEFAULT, PENDING_MAX);

	palert = &alert;

	k_alert_send(&alert);

	ret = k_alert_recv(&alert, K_FOREVER);

	zassert_equal(ret, 0, NULL);

	irq_offload(sync_entry, NULL);

	ret = k_alert_recv(&alert, K_FOREVER);

	zassert_equal(ret, 0, NULL);

	k_sem_give(&sync_sema);

	for (i = 0; i < 2; i++) {
		ret = k_alert_recv(&alert, K_FOREVER);

		zassert_equal(ret, 0, NULL);
	}
}

/**
 * @brief Test thread alert handler
 *
 * This test checks that the event handler is set up properly when
 * k_alert_init() is called with an event handler.  It shows that event
 * handlers are tied to the specified event and that the return value from the
 * handler affects whether the event wakes a task waiting upon that event.
 *
 * @see k_alert_init()
 */
void test_thread_alert_handler(void)
{
	/**TESTPOINT: alert handler ignore*/
	struct k_alert alert;
	int ret;

	/**TESTPOINT: init via k_alert_init*/
	k_alert_init(&alert, event_handler, PENDING_MAX);

	palert = &alert;

	k_sem_give(&sync_sema);

	ret = k_alert_recv(&alert, TIMEOUT);

	zassert_equal(ret, -EAGAIN, NULL);

	k_sem_give(&sync_sema);

	ret = k_alert_recv(&alert, TIMEOUT);

	zassert_equal(ret, 0, NULL);
}
/**
 * @}
 */


/**
 * Signal various events to a waiting task
 */
void signal_task(void *p1, void *p2, void *p3)
{
	k_sem_init(&sync_sema, 0, 1);

	k_sem_take(&sync_sema, K_FOREVER);
	k_alert_send(palert);
	irq_offload(sync_entry, NULL);

	k_sem_take(&sync_sema, K_FOREVER);
	k_alert_send(palert);
	irq_offload(sync_entry, NULL);

	k_sem_take(&sync_sema, K_FOREVER);
	handler_val = 0;
	k_alert_send(palert);

	k_sem_take(&sync_sema, K_FOREVER);
	handler_val = 1;
	k_alert_send(palert);
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

	/**TESTPOINT: thread-thread sync via alert*/
	k_thread_create(&sync_tdata, sync_tstack, STACK_SIZE,
				      signal_task, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(0), 0, 0);

	ztest_test_suite(alert_api,
			 ztest_unit_test(test_thread_alert_timeout),
			 ztest_unit_test(test_thread_alert_wait),
			 ztest_unit_test(test_thread_alert_handler),
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
	ztest_run_test_suite(alert_api);
}
