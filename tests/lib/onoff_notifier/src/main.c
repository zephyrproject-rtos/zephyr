/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/onoff.h>
#include <string.h>

static int onoff_status[10];
static unsigned int num_onoff;

static int notify_status[10];
static unsigned int num_notify;

struct service {
	struct onoff_manager onoff;
	onoff_notify_fn notify;
	int request_rv;
	int release_rv;
	int reset_rv;
	bool async;
	bool active;
};

static struct service service;
static struct onoff_notifier notifier;

static void notify_onoff(onoff_notify_fn notify,
			 int status)
{
	__ASSERT_NO_MSG(num_onoff < ARRAY_SIZE(onoff_status));
	printk("onoff notify %d\n", status);
	onoff_status[num_onoff++] = status;
	notify(&service.onoff, status);
}

static void settle_onoff(int res,
			 bool request)
{
	onoff_notify_fn notify = service.notify;

	__ASSERT_NO_MSG(notify != NULL);
	service.notify = NULL;
	service.active = (request && (res >= 0));
	notify_onoff(notify, res);
}

static void basic_start(struct onoff_manager *mp,
			onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);

	if (sp->async) {
		__ASSERT_NO_MSG(sp->notify == NULL);
		sp->notify = notify;
	} else {
		sp->active = (sp->request_rv >= 0);
		notify_onoff(notify, sp->request_rv);
	}
}

static void basic_stop(struct onoff_manager *mp,
		       onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);

	if (sp->async) {
		__ASSERT_NO_MSG(sp->notify == NULL);
		sp->notify = notify;
	} else {
		sp->active = false;
		notify_onoff(notify, sp->release_rv);
	}
}

static void basic_reset(struct onoff_manager *mp,
			onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);

	if (sp->async) {
		__ASSERT_NO_MSG(sp->notify == NULL);
		sp->notify = notify;
	} else {
		sp->active = false;
		notify_onoff(notify, sp->reset_rv);
	}
}

static struct onoff_transitions const transitions = {
	.start = basic_start,
	.stop = basic_stop,
	.reset = basic_reset,
};

static void notify_callback(struct onoff_notifier *np,
			    int status)
{
	__ASSERT_NO_MSG(num_notify < ARRAY_SIZE(notify_status));
	notify_status[num_notify++] = status;
}

static void reset_service(void)
{
	memset(onoff_status, 0, sizeof(onoff_status));
	num_onoff = 0;
	memset(notify_status, 0, sizeof(notify_status));
	num_notify = 0;

	service = (struct service){
		.onoff = ONOFF_MANAGER_INITIALIZER(&transitions),
	};

	notifier = (struct onoff_notifier)
		   ONOFF_NOTIFIER_INITIALIZER(&service.onoff, notify_callback);
}


static void replace_service_onoff(struct onoff_transitions *transitions)
{
	service.onoff.transitions = transitions;
}

static void test_basic(void)
{
	int rc;

	reset_service();

	zassert_false(service.active,
		      "unexp active");

	/* Immediate success expected */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 1,
		      "request failed");
	zassert_equal(num_onoff, 1,
		      "onoff not invoked");
	zassert_true(service.active,
		     "not active");
	zassert_equal(num_notify, 1,
		      "req not notified");
	zassert_equal(notify_status[0], 1,
		      "notification not on");

	/* No-effect error to re-request */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EALREADY,
		      "re-request failure");

	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, 0,
		      "release failed");
	zassert_false(service.active,
		      "still active");
	zassert_equal(num_notify, 2,
		      "rel not notified");
	zassert_equal(notify_status[1], 0,
		      "notification on");

	/* No-effect error to re-release */
	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, -EALREADY,
		      "re-release failure");
}

static void test_failed_request(void)
{
	int rc;

	reset_service();
	service.request_rv = -23;

	zassert_false(service.active,
		      "unexp active");

	/* Immediate failure expected */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request failed: %d", rc);
	zassert_equal(num_onoff, 1,
		      "onoff not invoked");
	zassert_false(service.active,
		      "active");

	/* Failures are persistent until service reset. */
	service.request_rv = 0;
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request failed: %d", rc);
}

static void test_async(void)
{
	int rc;

	reset_service();
	service.async = true;

	zassert_false(service.active,
		      "unexp active");

	/* No immediate success */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 0,
		      "request not incomplete");
	zassert_equal(num_onoff, 0,
		      "onoff premature");
	zassert_false(service.active,
		      "unexp active");
	zassert_equal(num_notify, 0,
		      "notify premature");

	/* Re-invocation at this point has no effect */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 0,
		      "request not incomplete");

	settle_onoff(service.request_rv, true);
	zassert_equal(num_onoff, 1,
		      "onoff premature");
	zassert_true(service.active,
		     "unexp inactive");
	zassert_equal(num_notify, 1,
		      "notify premature");

	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, 0,
		      "release failed: %d", rc);
	zassert_equal(num_onoff, 1,
		      "onoff premature");

	settle_onoff(service.request_rv, false);

	zassert_false(service.active,
		      "still active");
	zassert_equal(num_notify, 2,
		      "rel not notified");
	zassert_equal(notify_status[1], 0,
		      "notification on");
}

static void test_cancelled_request(void)
{
	int rc;

	reset_service();
	service.async = true;

	zassert_false(service.active,
		      "unexp active");

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 0,
		      "request not incomplete");
	zassert_equal(num_onoff, 0,
		      "onoff premature");
	zassert_false(service.active,
		      "unexp active");
	zassert_equal(num_notify, 0,
		      "notify premature");

	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, 0,
		      "request not incomplete");
	zassert_equal(num_onoff, 0,
		      "onoff premature");
	zassert_false(service.active,
		      "unexp active");
	zassert_equal(num_notify, 0,
		      "notify premature");

	/* Complete the initial request */
	settle_onoff(0, true);
	zassert_equal(num_onoff, 1,
		      "on not complete");
	zassert_equal(num_notify, 0,
		      "notify premature");
	zassert_not_equal(service.notify, NULL,
			  "stop transition not invoked");

	/* Complete the synthesized cancellation.  We should get one
	 * notification that the service is off.
	 */
	settle_onoff(0, false);
	zassert_equal(num_onoff, 2,
		      "off not complete");
	zassert_equal(num_notify, 1,
		      "notify not received");
	zassert_equal(notify_status[0], 0,
		      "notification on");
}

static void test_bicancelled_request(void)
{
	int rc;

	reset_service();
	service.async = true;

	zassert_false(service.active,
		      "unexp active");

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 0,
		      "request not incomplete");
	zassert_equal(num_onoff, 0,
		      "onoff premature");
	zassert_false(service.active,
		      "unexp active");
	zassert_equal(num_notify, 0,
		      "notify premature");

	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, 0,
		      "request not incomplete");
	zassert_equal(num_onoff, 0,
		      "onoff premature");
	zassert_false(service.active,
		      "unexp active");
	zassert_equal(num_notify, 0,
		      "notify premature");

	/* Issue a request which cancels the pending release */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 0,
		      "request not incomplete");
	zassert_equal(num_onoff, 0,
		      "onoff premature");
	zassert_false(service.active,
		      "unexp active");
	zassert_equal(num_notify, 0,
		      "notify premature");

	/* Complete the initial request.  The intermediary release was
	 * cancelled before it could be initiated.
	 */
	settle_onoff(0, true);
	zassert_equal(num_onoff, 1,
		      "on not complete");
	zassert_equal(num_notify, 1,
		      "notify premature");
	zassert_equal(notify_status[0], 1,
		      "notification on");
	zassert_equal(service.notify, NULL,
		      "stop transition queued");
}

static void test_cancelled_release(void)
{
	int rc;

	reset_service();

	zassert_false(service.active,
		      "unexp active");

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 1,
		      "request not complete");
	zassert_equal(num_onoff, 1,
		      "onoff failed");
	zassert_equal(num_notify, 1,
		      "notify failed");
	zassert_equal(notify_status[0], 1,
		      "notify failed");
	zassert_true(service.active,
		     "exp active");

	service.async = true;

	/* Issue a release, which will block. */
	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, 0,
		      "request complete");
	zassert_equal(num_onoff, 1,
		      "onoff premature");
	zassert_equal(num_notify, 1,
		      "notify premature");

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 0,
		      "request not complete");

	/* Complete the initial release */
	settle_onoff(0, false);
	zassert_equal(num_onoff, 2,
		      "on not complete");
	zassert_equal(num_notify, 1,
		      "notify premature");
	zassert_not_equal(service.notify, NULL,
			  "stop transition not invoked");

	/* Complete the synthesized request */
	settle_onoff(0, true);
	zassert_equal(num_onoff, 3,
		      "off not complete");
	zassert_equal(num_notify, 2,
		      "notify not received");
	zassert_equal(notify_status[0], 1,
		      "notification");
	zassert_equal(notify_status[1], 1,
		      "renotification");
}

static void test_bicancelled_release(void)
{
	int rc;

	reset_service();

	zassert_false(service.active,
		      "unexp active");

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 1,
		      "request not complete");
	zassert_equal(num_onoff, 1,
		      "onoff failed");
	zassert_equal(num_notify, 1,
		      "notify failed");
	zassert_equal(notify_status[0], 1,
		      "notify failed");
	zassert_true(service.active,
		     "exp active");

	service.async = true;

	/* Issue a release, which will block. */
	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, 0,
		      "request complete");
	zassert_equal(num_onoff, 1,
		      "onoff premature");
	zassert_equal(num_notify, 1,
		      "notify premature");

	/* Issue a request to cancel the release */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, 0,
		      "request not complete");

	/* Issue a second release to cancel the pending request */
	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, 0,
		      "request not complete");
	zassert_equal(num_onoff, 1,
		      "onoff premature");
	zassert_equal(num_notify, 1,
		      "notify premature");

	/* Complete the initial release */
	settle_onoff(0, false);
	zassert_equal(num_onoff, 2,
		      "on not complete");
	zassert_equal(num_notify, 2,
		      "notify ok");
	zassert_equal(service.notify, NULL,
		      "start transition pending");
	zassert_equal(notify_status[1], 0,
		      "notify failed");
}

static void test_basic_reset(void)
{
	int rc;

	reset_service();
	service.request_rv = -23;

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request error");
	zassert_equal(num_notify, 1,
		      "notify wrong");
	zassert_equal(notify_status[0], service.request_rv,
		      "notify status wrong");

	/* Non-reset operations in an error state produce an error. */
	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, -EIO,
		      "release check");
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request check");

	rc = onoff_notifier_reset(&notifier);
	zassert_equal(rc, 0,
		      "reset unsupported failed: %d", rc);
	zassert_equal(num_notify, 2,
		      "notify wrong: %d", num_notify);
	zassert_equal(notify_status[1], 0,
		      "reset failed");

	/* Re-reset is rejected */
	rc = onoff_notifier_reset(&notifier);
	zassert_equal(rc, -EALREADY,
		      "re-reset failed");

}

static void test_unsupported_reset(void)
{
	int rc;

	reset_service();

	struct onoff_transitions reset_transitions = *service.onoff.transitions;

	reset_transitions.reset = NULL;
	replace_service_onoff(&reset_transitions);
	service.request_rv = -23;

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request error");
	zassert_equal(num_notify, 1,
		      "notify wrong");
	zassert_equal(notify_status[0], service.request_rv,
		      "notify status wrong");

	/* Non-reset operations in an error state produce an error. */
	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, -EIO,
		      "release check");
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request check");

	/* Reset fails if service can't be reset */
	rc = onoff_notifier_reset(&notifier);
	zassert_equal(rc, -EIO,
		      "reset unsupported failed");
	zassert_equal(num_notify, 2,
		      "notify wrong");
	zassert_equal(notify_status[1], -ENOTSUP,
		      "reset status wrong: %d", notify_status[1]);
}

static void test_already_reset(void)
{
	int rc;

	reset_service();
	service.request_rv = -23;

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request error");
	zassert_equal(num_notify, 1,
		      "notify wrong");
	zassert_equal(notify_status[0], service.request_rv,
		      "notify status wrong");

	zassert_true(onoff_has_error(&service.onoff),
		     "no error?");

	/* Clear the underlying error as if from another process */
	struct onoff_client cli;

	onoff_client_init_spinwait(&cli);
	rc = onoff_reset(&service.onoff, &cli);
	zassert_equal(rc, 0,
		      "reset failed");
	zassert_false(onoff_has_error(&service.onoff),
		      "no error?");

	onoff_client_init_spinwait(&cli);
	rc = onoff_reset(&service.onoff, &cli);
	zassert_equal(rc, -EALREADY,
		      "re-reset succeeded");

	/* Notifier reset should still succeed. */
	rc = onoff_notifier_reset(&notifier);
	zassert_equal(rc, 0,
		      "request error");
	zassert_equal(num_notify, 2,
		      "notify wrong");
	zassert_equal(notify_status[1], 0,
		      "notify status wrong");
}

static void test_async_reset(void)
{
	int rc;

	reset_service();
	service.request_rv = -23;

	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EIO,
		      "request error");
	zassert_equal(num_notify, 1,
		      "notify wrong");
	zassert_equal(notify_status[0], service.request_rv,
		      "notify status wrong");

	zassert_true(onoff_has_error(&service.onoff),
		     "no error?");

	service.async = true;

	/* Notifier reset should be acceptable. */
	rc = onoff_notifier_reset(&notifier);
	zassert_equal(rc, 0,
		      "request error");
	zassert_equal(num_notify, 1,
		      "notify wrong");

	/* Other operations should be rejected while reset is
	 * unresolved.
	 */
	rc = onoff_notifier_request(&notifier);
	zassert_equal(rc, -EWOULDBLOCK,
		      "request failed");
	rc = onoff_notifier_release(&notifier);
	zassert_equal(rc, -EWOULDBLOCK,
		      "release failed");

	settle_onoff(0, false);
	zassert_equal(num_notify, 2,
		      "notify wrong");
	zassert_equal(notify_status[1], 0,
		      "notify status wrong");
}

void test_main(void)
{
	ztest_test_suite(onoff_notifier_api,
			 ztest_unit_test(test_basic),
			 ztest_unit_test(test_async),
			 ztest_unit_test(test_failed_request),
			 ztest_unit_test(test_cancelled_request),
			 ztest_unit_test(test_bicancelled_request),
			 ztest_unit_test(test_cancelled_release),
			 ztest_unit_test(test_bicancelled_release),
			 ztest_unit_test(test_basic_reset),
			 ztest_unit_test(test_unsupported_reset),
			 ztest_unit_test(test_already_reset),
			 ztest_unit_test(test_async_reset)
			 );
	ztest_run_test_suite(onoff_notifier_api);
}
