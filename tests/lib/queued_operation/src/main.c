/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/queued_operation.h>

struct service;

struct operation {
	struct queued_operation operation;
	void (*callback)(struct service *sp,
			 struct operation *op,
			 void *ud);
	void *user_data;
};

struct service {
	/* State of the manager */
	struct queued_operation_manager manager;

	/* State for an on-off service optionally used by the manager. */
	struct onoff_manager onoff;

	/* Value to return from basic_request handler. */
	int onoff_request_rv;

	/* Value to return from basic_release handler. */
	int onoff_release_rv;

	/* Value to return from basic_reset handler. */
	int onoff_reset_rv;

	/* Notifier to use when async_onoff is set. */
	onoff_notify_fn onoff_notify;
	/* The current operation cast for this service type.  Null if
	 * service is idle.
	 */
	struct operation *current;

	/* Value to return from service_impl_validate() */
	int validate_rv;

	/* Value to return from service_impl_validate()
	 *
	 * This is incremented before each synchronous finalization by
	 * service_impl_callback.
	 */
	int process_rv;

	/* Parameters passed to test_callback */
	struct operation *callback_op;
	int callback_res;

	/* Count of process submissions since reset. */
	size_t process_cnt;

	/* Test-specific data associated with the service. */
	void *data;

	/* If set defer notification of onoff operation.
	 *
	 * The callback to invoke will be stored in onoff_notify.
	 */
	bool async_onoff;

	/* If set inhibit synchronous completion. */
	bool async;

	/* Set to indicate that the lass process() call provided an
	 * operation.
	 */
	bool active;
};

static void basic_start(struct onoff_manager *mp,
			onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);

	if (sp->async_onoff) {
		__ASSERT_NO_MSG(sp->onoff_notify == NULL);
		sp->onoff_notify = notify;
	} else {
		sp->active = sp->onoff_request_rv >= 0;
		notify(mp, sp->onoff_request_rv);
	}
}

static void basic_stop(struct onoff_manager *mp,
		       onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);

	if (sp->async_onoff) {
		__ASSERT_NO_MSG(sp->onoff_notify == NULL);
		sp->onoff_notify = notify;
	} else {
		sp->active = false;
		notify(mp, sp->onoff_release_rv);
	}
}

static void basic_reset(struct onoff_manager *mp,
			onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);

	if (sp->async_onoff) {
		__ASSERT_NO_MSG(sp->onoff_notify == NULL);
		sp->onoff_notify = notify;
	} else {
		sp->active = false;
		notify(mp, sp->onoff_reset_rv);
	}
}

static struct onoff_transitions const basic_onoff_transitions = {
	.start = basic_start,
	.stop = basic_stop,
	.reset = basic_reset,
};

typedef void (*service_callback)(struct service *sp,
				 struct operation *op,
				 int res);

static void test_callback(struct service *sp,
			  struct operation *op,
			  int res)
{
	sp->callback_op = op;
	sp->callback_res = res;
	if (op->callback) {
		op->callback(sp, op, op->user_data);
	}
}

static inline void operation_init_spinwait(struct operation *op)
{
	*op = (struct operation){};
	sys_notify_init_spinwait(&op->operation.notify);
}

static inline void operation_init_signal(struct operation *op,
					 struct k_poll_signal *sigp)
{
	*op = (struct operation){};
	sys_notify_init_signal(&op->operation.notify, sigp);
}

static inline void operation_init_callback(struct operation *op,
					   service_callback handler)
{
	*op = (struct operation){};
	sys_notify_init_callback(&op->operation.notify,
				 (sys_notify_generic_callback)handler);
}

static int service_submit(struct service *sp,
			  struct operation *op,
			  int priority)
{
	return queued_operation_submit(&sp->manager, &op->operation, priority);
}

static int service_cancel(struct service *sp,
			  struct operation *op)
{
	return queued_operation_cancel(&sp->manager, &op->operation);
}

static int service_impl_validate(struct queued_operation_manager *mgr,
				 struct queued_operation *op)
{
	struct service *sp = CONTAINER_OF(mgr, struct service, manager);

	return sp->validate_rv;
}

static bool service_has_error(struct service *sp)
{
	return queued_operation_has_error(&sp->manager);
}

static int service_reset(struct service *sp,
			 struct onoff_client *oocli)
{
	return queued_operation_reset(&sp->manager, oocli);
}

static void service_impl_callback(struct queued_operation_manager *mgr,
				  struct queued_operation *op,
				  sys_notify_generic_callback cb)
{
	service_callback handler = (service_callback)cb;
	struct service *sp = CONTAINER_OF(mgr, struct service, manager);
	struct operation *sop = CONTAINER_OF(op, struct operation, operation);
	int res = -EINPROGRESS;

	zassert_equal(queued_operation_fetch_result(op, &res), 0,
		      "callback before finalized");
	handler(sp, sop, res);
}

/* Split out finalization to support async testing. */
static void service_finalize(struct service *sp,
			     int res)
{
	struct queued_operation *op = &sp->current->operation;

	sp->current = NULL;
	(void)op;
	queued_operation_finalize(&sp->manager, res);
}

static void service_impl_process(struct queued_operation_manager *mgr,
				 struct queued_operation *op)
{
	struct service *sp = CONTAINER_OF(mgr, struct service, manager);

	zassert_equal(sp->current, NULL,
		      "process collision");

	sp->process_cnt++;
	sp->active = (op != NULL);
	if (sp->active) {
		sp->current = CONTAINER_OF(op, struct operation, operation);
		if (!sp->async) {
			service_finalize(sp, ++sp->process_rv);
		}
	}
}

static struct queued_operation_functions const service_vtable = {
	.validate = service_impl_validate,
	.callback = service_impl_callback,
	.process = service_impl_process,
};
/* Live copy, mutated for testing. */
static struct queued_operation_functions vtable;

static struct service service = {
	.manager = QUEUED_OPERATION_MANAGER_INITIALIZER(&vtable,
							&service.onoff),
	.onoff = {
		.transitions = &basic_onoff_transitions,
	},
};

static void service_onoff_notify(int res)
{
	onoff_notify_fn notify = service.onoff_notify;

	__ASSERT_NO_MSG(notify != NULL);
	service.onoff_notify = NULL;

	notify(&service.onoff, res);
}

static void reset_service(bool onoff)
{
	vtable = service_vtable;
	service = (struct service){
		.manager = QUEUED_OPERATION_MANAGER_INITIALIZER(&vtable,
								&service.onoff),
		.onoff = {
			.transitions = &basic_onoff_transitions,
		},
	};

	if (!onoff) {
		service.manager.onoff = NULL;
	}
}

static void replace_service_onoff(struct onoff_transitions *transitions)
{
	service.onoff.transitions = transitions;
}

static void test_notification_spinwait(void)
{
	struct operation operation;
	struct operation *op = &operation;
	struct sys_notify *np = &op->operation.notify;
	int res = 0;
	int rc = 0;

	reset_service(true);

	operation_init_spinwait(&operation);
	zassert_equal(sys_notify_fetch_result(np, &res), -EAGAIN,
		      "failed spinwait unfinalized");

	rc = service_submit(&service, op, 0);
	zassert_equal(rc, service.validate_rv,
		      "submit spinwait failed: %d != %d", rc,
		      service.validate_rv);
	zassert_equal(sys_notify_fetch_result(np, &res), 0,
		      "failed spinwait fetch");
	zassert_equal(res, service.process_rv,
		      "failed spinwait result");

	zassert_false(service.active, "service not idled");
}

static void test_notification_signal(void)
{
	struct operation operation;
	struct operation *op = &operation;
	struct sys_notify *np = &op->operation.notify;
	struct k_poll_signal sig;
	unsigned int signaled;
	int res = 0;
	int rc = 0;

	reset_service(false);

	k_poll_signal_init(&sig);
	operation_init_signal(op, &sig);
	zassert_equal(sys_notify_fetch_result(np, &res), -EAGAIN,
		      "failed signal unfinalized");
	k_poll_signal_check(&sig, &signaled, &res);
	zassert_equal(signaled, 0,
		      "failed signal unsignaled");

	service.process_rv = 23;
	rc = service_submit(&service, op, 0);
	zassert_equal(rc, 0,
		      "submit signal failed: %d", rc);
	zassert_equal(sys_notify_fetch_result(np, &res), 0,
		      "failed signal fetch");
	zassert_equal(res, service.process_rv,
		      "failed signal result");
	k_poll_signal_check(&sig, &signaled, &res);
	zassert_equal(signaled, 1,
		      "failed signal signaled");
	zassert_equal(res, service.process_rv,
		      "failed signal signal result");
}

static void test_notification_callback(void)
{
	struct operation operation;
	struct operation *op = &operation;
	struct service *sp = &service;
	struct sys_notify *np = &op->operation.notify;
	struct k_poll_signal sig;
	int res = 0;
	int rc = 0;

	reset_service(false);

	k_poll_signal_init(&sig);
	operation_init_callback(op, test_callback);
	zassert_equal(sys_notify_fetch_result(np, &res), -EAGAIN,
		      "failed callback unfinalized");
	zassert_equal(sp->callback_op, NULL,
		      "failed callback pre-check");

	service.process_rv = 142;
	rc = service_submit(&service, op, 0);
	zassert_equal(rc, 0,
		      "submit callback failed: %d", rc);
	zassert_equal(sys_notify_fetch_result(np, &res), 0,
		      "failed callback fetch");
	zassert_equal(res, service.process_rv,
		      "failed callback result");
	zassert_equal(sp->callback_op, op,
		      "failed callback captured op");
	zassert_equal(sp->callback_res, service.process_rv,
		      "failed callback captured res");
}

struct pri_order {
	int priority;
	size_t ordinal;
};

static void test_sync_priority(void)
{
	struct pri_order const pri_order[] = {
		{ 0, 0 }, /* first because it gets grabbed when submitted */
		/* rest in FIFO within priority */
		{ -1, 2 },
		{ 1, 4 },
		{ -2, 1 },
		{ 2, 6 },
		{ 1, 5 },
		{ 0, 3 },
	};
	struct operation operation[ARRAY_SIZE(pri_order)];
	struct sys_notify *np[ARRAY_SIZE(operation)];
	int res = -EINPROGRESS;
	int rc;

	/* Reset the service, and tell it to not finalize operations
	 * synchronously (so we can build up a queue).
	 */
	reset_service(false);
	service.async = true;

	for (uint32_t i = 0; i < ARRAY_SIZE(operation); ++i) {
		operation_init_spinwait(&operation[i]);
		np[i] = &operation[i].operation.notify;
		rc = service_submit(&service, &operation[i],
				    pri_order[i].priority);
		zassert_equal(rc, 0,
			      "submit op%u failed: %d", i, rc);
		zassert_equal(sys_notify_fetch_result(np[i], &res), -EAGAIN,
			      "op%u finalized!", i);
	}

	zassert_equal(service.current, &operation[0],
		      "submit op0 didn't process");

	/* Enable synchronous finalization and kick off the first
	 * entry.  All the others will execute immediately.
	 */
	service.async = false;
	service_finalize(&service, service.process_rv);

	for (uint32_t i = 0; i < ARRAY_SIZE(operation); ++i) {
		size_t ordinal = pri_order[i].ordinal;

		zassert_equal(sys_notify_fetch_result(np[i], &res), 0,
			      "op%u unfinalized", i);
		zassert_equal(res, ordinal,
			      "op%u wrong order: %d != %u", i, res, ordinal);
	}
}

static void test_special_priority(void)
{
	struct pri_order const pri_order[] = {
		{ 0, 0 }, /* first because it gets grabbed when submitted */
		/* rest gets tricky */
		{ QUEUED_OPERATION_PRIORITY_APPEND, 3 },
		{ INT8_MAX, 4 },
		{ INT8_MIN, 2 },
		{ QUEUED_OPERATION_PRIORITY_PREPEND, 1 },
		{ QUEUED_OPERATION_PRIORITY_APPEND, 5 },
	};
	struct operation operation[ARRAY_SIZE(pri_order)];
	struct sys_notify *np[ARRAY_SIZE(operation)];
	int res = -EINPROGRESS;
	int rc;

	/* Reset the service, and tell it to not finalize operations
	 * synchronously (so we can build up a queue).
	 */
	reset_service(false);
	service.async = true;

	for (uint32_t i = 0; i < ARRAY_SIZE(operation); ++i) {
		operation_init_spinwait(&operation[i]);
		np[i] = &operation[i].operation.notify;
		rc = service_submit(&service, &operation[i],
				    pri_order[i].priority);
		zassert_equal(rc, 0,
			      "submit op%u failed: %d", i, rc);
		zassert_equal(sys_notify_fetch_result(np[i], &res), -EAGAIN,
			      "op%u finalized!", i);
	}

	zassert_equal(service.current, &operation[0],
		      "submit op0 didn't process");

	/* Enable synchronous finalization and kick off the first
	 * entry.  All the others will execute immediately.
	 */
	service.async = false;
	service_finalize(&service, service.process_rv);

	for (uint32_t i = 0; i < ARRAY_SIZE(operation); ++i) {
		size_t ordinal = pri_order[i].ordinal;

		zassert_equal(sys_notify_fetch_result(np[i], &res), 0,
			      "op%u unfinalized", i);
		zassert_equal(res, ordinal,
			      "op%u wrong order: %d != %u", i, res, ordinal);
	}
}

struct delayed_submit {
	struct operation *op;
	int priority;
};

static void test_delayed_submit(struct service *sp,
				struct operation *op,
				void *ud)
{
	struct delayed_submit *dsp = ud;
	int rc = service_submit(sp, dsp->op, dsp->priority);

	zassert_equal(rc, 0,
		      "delayed submit failed: %d", rc);
}

static void test_resubmit_priority(void)
{
	struct pri_order const pri_order[] = {
		/* first because it gets grabbed when submitted */
		{ 0, 0 },
		/* delayed by submit of higher priority during callback */
		{ 0, 2 },
		/* submitted during completion of op0 */
		{ -1, 1 },
	};
	size_t di = ARRAY_SIZE(pri_order) - 1;
	struct operation operation[ARRAY_SIZE(pri_order)];
	struct sys_notify *np[ARRAY_SIZE(operation)];
	int res = -EINPROGRESS;
	int rc;

	/* Queue two operations, but in the callback for the first
	 * schedule a third operation that has higher priority.
	 */
	reset_service(false);
	service.async = true;

	for (uint32_t i = 0; i <= di; ++i) {
		operation_init_callback(&operation[i], test_callback);
		np[i] = &operation[i].operation.notify;
		if (i < di) {
			rc = service_submit(&service, &operation[i], 0);
			zassert_equal(rc, 0,
				      "submit op%u failed: %d", i, rc);
			zassert_equal(sys_notify_fetch_result(np[i], &res),
				      -EAGAIN,
				      "op%u finalized!", i);
		}
	}

	struct delayed_submit ds = {
		.op = &operation[di],
		.priority = pri_order[di].priority,
	};
	operation[0].callback = test_delayed_submit;
	operation[0].user_data = &ds;

	/* Enable synchronous finalization and kick off the first
	 * entry.  All the others will execute immediately.
	 */
	service.async = false;
	service_finalize(&service, service.process_rv);

	zassert_equal(service.process_cnt, ARRAY_SIZE(operation),
		      "not all processed once: %d != %d",
		      ARRAY_SIZE(operation), service.process_cnt);

	for (uint32_t i = 0; i < ARRAY_SIZE(operation); ++i) {
		size_t ordinal = pri_order[i].ordinal;

		zassert_equal(sys_notify_fetch_result(np[i], &res), 0,
			      "op%u unfinalized", i);
		zassert_equal(res, ordinal,
			      "op%u wrong order: %d != %u", i, res, ordinal);
	}
}

static void test_missing_validation(void)
{
	struct operation operation;
	struct operation *op = &operation;
	struct sys_notify *np = &op->operation.notify;
	int res = 0;
	int rc = 0;

	reset_service(false);
	vtable.validate = NULL;

	operation_init_spinwait(&operation);
	zassert_equal(sys_notify_fetch_result(np, &res), -EAGAIN,
		      "failed spinwait unfinalized");

	rc = service_submit(&service, op, 0);
	zassert_equal(rc, 0,
		      "submit spinwait failed: %d", rc);
	zassert_equal(sys_notify_fetch_result(np, &res), 0,
		      "failed spinwait fetch");
	zassert_equal(res, service.process_rv,
		      "failed spinwait result");
}

static void test_success_validation(void)
{
	struct operation operation;
	struct operation *op = &operation;
	struct sys_notify *np = &op->operation.notify;
	int res = 0;
	int rc = 0;

	reset_service(false);
	service.validate_rv = 57;

	operation_init_spinwait(&operation);
	zassert_equal(sys_notify_fetch_result(np, &res), -EAGAIN,
		      "failed spinwait unfinalized");

	rc = service_submit(&service, op, 0);
	zassert_equal(rc, service.validate_rv,
		      "submit validation did not succeed as expected: %d", rc);
}

static void test_failed_validation(void)
{
	struct operation operation;
	struct operation *op = &operation;
	struct sys_notify *np = &op->operation.notify;
	int res = 0;
	int rc = 0;

	reset_service(false);
	service.validate_rv = -EINVAL;

	operation_init_spinwait(&operation);
	zassert_equal(sys_notify_fetch_result(np, &res), -EAGAIN,
		      "failed spinwait unfinalized");

	rc = service_submit(&service, op, 0);
	zassert_equal(rc, service.validate_rv,
		      "submit validation did not fail as expected: %d", rc);
}

static void test_callback_validation(void)
{
	struct operation operation;
	struct operation *op = &operation;
	int expect = -ENOTSUP;
	int rc = 0;

	reset_service(false);
	vtable.callback = NULL;

	operation_init_callback(&operation, test_callback);
	rc = service_submit(&service, op, 0);
	zassert_equal(rc, expect,
		      "unsupported callback check failed: %d != %d",
		      rc, expect);
}

static void test_priority_validation(void)
{
	struct operation operation;
	struct operation *op = &operation;
	int expect = -EINVAL;
	int rc = 0;

	reset_service(false);

	operation_init_callback(&operation, test_callback);
	rc = service_submit(&service, op, 128);
	zassert_equal(rc, expect,
		      "unsupported priority check failed: %d != %d",
		      rc, expect);
}

static void test_cancel_active(void)
{
	struct operation operation;
	struct operation *op = &operation;
	int expect = -EINPROGRESS;
	int rc = 0;

	reset_service(false);
	service.async = true;
	service.validate_rv = 152;

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);
	zassert_equal(rc, service.validate_rv,
		      "submit failed: %d != %d", rc, service.validate_rv);

	rc = service_cancel(&service, op);
	zassert_equal(rc, expect,
		      "cancel failed: %d != %d", rc, expect);
}

static void test_cancel_inactive(void)
{
	struct operation operation[2];
	struct sys_notify *np[ARRAY_SIZE(operation)];
	struct operation *op1 = &operation[1];
	int res;
	int rc = 0;

	reset_service(false);
	service.async = true;

	/* Set up two operations, but only submit the first. */
	for (uint32_t i = 0; i < ARRAY_SIZE(operation); ++i) {
		operation_init_spinwait(&operation[i]);
		np[i] = &operation[i].operation.notify;
		if (i == 0) {
			rc = service_submit(&service, &operation[i], 0);
			zassert_equal(rc, service.validate_rv,
				      "submit failed: %d != %d",
				      rc, service.validate_rv);
		}
	}

	zassert_equal(service.current, &operation[0],
		      "current not op0");

	zassert_equal(sys_notify_fetch_result(np[1], &res), -EAGAIN,
		      "op1 finalized!");

	/* Verify attempt to cancel unsubmitted operation. */
	rc = service_cancel(&service, op1);
	zassert_equal(rc, -EINVAL,
		      "cancel failed: %d != %d", rc, -EINVAL);

	/* Submit, then verify cancel succeeds. */
	rc = service_submit(&service, op1, 0);
	zassert_equal(rc, service.validate_rv,
		      "submit failed: %d != %d", rc, service.validate_rv);

	zassert_equal(sys_notify_fetch_result(np[1], &res), -EAGAIN,
		      "op1 finalized!");

	rc = service_cancel(&service, op1);
	zassert_equal(rc, 0,
		      "cancel failed: %d", rc);

	zassert_equal(sys_notify_fetch_result(np[1], &res), 0,
		      "op1 NOT finalized");
	zassert_equal(res, -ECANCELED,
		      "op1 cancel result unexpected: %d", res);

	service.async = false;
	service_finalize(&service, service.process_rv);
	zassert_equal(service.process_cnt, 1,
		      "too many processed");
}

static void test_async_idle(void)
{
	struct operation operation;

	reset_service(true);
	service.async = true;
	service.process_rv = 142;

	operation_init_spinwait(&operation);
	service_submit(&service, &operation, 0);
	service_finalize(&service, service.process_rv);
	zassert_false(service.active, "service not idled");
}

static void test_onoff_success(void)
{
	struct operation operation;
	struct operation *op = &operation;
	struct sys_notify *np = &op->operation.notify;
	int res = 0;
	int rc = 0;

	reset_service(true);
	service.process_rv = 23;
	service.async_onoff = true;

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);
	zassert_equal(rc, service.validate_rv,
		      "submit spinwait failed: %d != %d", rc,
		      service.validate_rv);
	zassert_equal(service.process_cnt, 0,
		      "unexpected process");
	zassert_equal(sys_notify_fetch_result(np, &res), -EAGAIN,
		      "unexpected fetch succeeded");
	zassert_not_equal(service.onoff_notify, NULL,
			  "unexpected notifier");

	service.active = true;
	service.async_onoff = false;
	service_onoff_notify(0);

	zassert_equal(service.process_cnt, 1,
		      "unexpected process");

	zassert_equal(sys_notify_fetch_result(np, &res), 0,
		      "failed spinwait fetch");
	zassert_equal(res, service.process_rv,
		      "failed spinwait result");

	zassert_false(service.active, "service not idled");
}

static void test_onoff_start_sync_failure(void)
{
	struct onoff_manager *oosrv;
	struct onoff_client oocli;
	struct operation operation;
	struct operation *op = &operation;
	struct sys_notify *np = &op->operation.notify;
	int res = 0;
	int rc = 0;

	reset_service(true);

	/* Force onoff service into error state. */
	service.onoff_request_rv = -14;

	oosrv = service.manager.onoff;
	oocli = (struct onoff_client){};
	sys_notify_init_spinwait(&oocli.notify);

	/* Request will succeed, transition will fail putting service
	 * into error state, which will cause a failure when the
	 * queued operation manager attempts to start the service.
	 */
	rc = onoff_request(oosrv, &oocli);
	zassert_equal(rc, 0,
		      "oo req: %d", rc);
	zassert_equal(sys_notify_fetch_result(&oocli.notify, &res), 0,
		      "failed spinwait fetch");
	zassert_equal(res, service.onoff_request_rv,
		      "res: %d", rc);
	zassert_true(onoff_has_error(oosrv),
		     "onoff error");

	service.onoff_request_rv = 0;

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);
	zassert_equal(rc, service.validate_rv,
		      "submit spinwait failed: %d != %d", rc,
		      service.validate_rv);

	zassert_equal(sys_notify_fetch_result(np, &res), 0,
		      "failed spinwait fetch");
	zassert_equal(res, -ENODEV,
		      "failed spinwait result: %d", res);

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);

}

static void test_onoff_start_failure(void)
{
	struct operation operation[2];
	struct sys_notify *np[ARRAY_SIZE(operation)];
	int onoff_res = -13;
	int res = 0;
	int rc = 0;

	reset_service(true);
	service.async_onoff = true;

	/* Queue two operations that will block on onoff start */
	for (uint32_t idx = 0; idx < ARRAY_SIZE(operation); ++idx) {
		np[idx] = &operation[idx].operation.notify;
		operation_init_spinwait(&operation[idx]);

		rc = service_submit(&service, &operation[idx], 0);
		zassert_equal(rc, service.validate_rv,
			      "submit spinwait %u failed: %d != %d", idx,
			      rc, service.validate_rv);
	}

	zassert_equal(service.process_cnt, 0,
		      "unexpected process");
	for (uint32_t idx = 0; idx < ARRAY_SIZE(operation); ++idx) {
		zassert_equal(sys_notify_fetch_result(np[idx], &res), -EAGAIN,
			      "unexpected fetch %u succeeded", idx);
	}
	zassert_not_equal(service.onoff_notify, NULL,
			  "unexpected notifier");

	/* Fail the start */
	service.async_onoff = false;
	service_onoff_notify(onoff_res);

	zassert_equal(service.process_cnt, 0,
		      "unexpected process");

	for (uint32_t idx = 0; idx < ARRAY_SIZE(operation); ++idx) {
		zassert_equal(sys_notify_fetch_result(np[idx], &res), 0,
			      "fetch %u failed", idx);
		/* TBD: provide access to onoff result code? */
		zassert_equal(res, -ENODEV,
			      "fetch %u value failed", idx);
	}
}

/* Data used to submit an operation during an onoff transition. */
struct onoff_restart_data {
	struct operation *op;
	int res;
	bool invoked;
};

/* Mutate the operation list during a stop to force a restart.  */
static void onoff_restart_stop(struct onoff_manager *mp,
			       onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);
	struct onoff_restart_data *dp = sp->data;

	if (dp) {
		int rc = service_submit(sp, dp->op, 0);

		zassert_equal(rc, sp->validate_rv,
			      "submit spinwait failed: %d != %d",
			      rc, sp->validate_rv);
		sp->data = NULL;
		dp->invoked = true;
	}

	basic_stop(mp, notify);
}

static void test_onoff_restart(void)
{
	struct operation operation[2];
	struct sys_notify *np[ARRAY_SIZE(operation)];
	int res = 0;
	int rc = 0;

	reset_service(true);

	struct onoff_transitions onoff_transitions = *service.onoff.transitions;
	struct onoff_restart_data stop_data = {
		.op = &operation[1],
	};

	service.data = &stop_data;
	onoff_transitions.stop = onoff_restart_stop;
	replace_service_onoff(&onoff_transitions);

	/* Initialize two operations.  The first is submitted, onoff
	 * starts, invokes the first, then stops.  During the stop the
	 * second is queued, which causes a restart when the stop
	 * completes.
	 */
	for (uint32_t idx = 0; idx < ARRAY_SIZE(operation); ++idx) {
		np[idx] = &operation[idx].operation.notify;
		operation_init_spinwait(&operation[idx]);

	}

	rc = service_submit(&service, &operation[0], 0);
	zassert_equal(rc, service.validate_rv,
		      "submit spinwait 0 failed: %d != %d",
		      rc, service.validate_rv);

	zassert_equal(service.process_cnt, 2,
		      "unexpected process");

	zassert_equal(stop_data.invoked, true,
		      "stop mock not invoked");

	for (uint32_t idx = 0; idx < ARRAY_SIZE(operation); ++idx) {
		zassert_equal(sys_notify_fetch_result(np[idx], &res), 0,
			      "failed spinwait fetch");
		zassert_equal(res, 1 + idx,
			      "failed spinwait result");
	}
}

/* Mutate the operation list during a stop to force a restart.  */
static void onoff_stop_failure_stop(struct onoff_manager *mp,
				    onoff_notify_fn notify)
{
	struct service *sp = CONTAINER_OF(mp, struct service, onoff);
	struct onoff_restart_data *dp = sp->data;
	int rc = service_submit(sp, dp->op, 0);

	zassert_equal(rc, sp->validate_rv,
		      "submit spinwait failed: %d != %d",
		      rc, sp->validate_rv);
	dp->invoked = true;
	sp->onoff_release_rv = dp->res;

	basic_stop(mp, notify);
}

static void test_onoff_stop_failure(void)
{
	struct operation operation[2];
	struct sys_notify *np[ARRAY_SIZE(operation)];
	int res = 0;
	int rc = 0;

	reset_service(true);

	struct onoff_transitions onoff_transitions = *service.onoff.transitions;
	struct onoff_restart_data stop_data = {
		.op = &operation[1],
		.res = -14,
	};

	service.data = &stop_data;
	onoff_transitions.stop = onoff_stop_failure_stop;
	replace_service_onoff(&onoff_transitions);

	/* Initialize two operations.  The first is submitted, onoff
	 * starts, invokes the first, then stops.  During the stop the
	 * second is queued, but the stop operation forces an error.
	 */
	for (uint32_t idx = 0; idx < ARRAY_SIZE(operation); ++idx) {
		np[idx] = &operation[idx].operation.notify;
		operation_init_spinwait(&operation[idx]);
	}

	rc = service_submit(&service, &operation[0], 0);
	zassert_equal(rc, service.validate_rv,
		      "submit spinwait 0 failed: %d != %d",
		      rc, service.validate_rv);

	zassert_equal(service.process_cnt, 1,
		      "unexpected process");
	zassert_equal(stop_data.invoked, true,
		      "stop mock not invoked");

	zassert_equal(sys_notify_fetch_result(np[0], &res), 0,
		      "failed spinwait 0 fetch");
	zassert_equal(res, service.process_rv,
		      "failed spinwait 0 result");
	zassert_equal(sys_notify_fetch_result(np[1], &res), 0,
		      "failed spinwait 1 fetch");
	zassert_equal(res, -ENODEV,
		      "failed spinwait 1 result");

	/* Verify that resubmits also return failure */

	operation_init_spinwait(&operation[0]);
	rc = service_submit(&service, &operation[0], 0);
	zassert_equal(rc, -ENODEV,
		      "failed error submit");
}

static void test_has_error(void)
{
	struct operation operation;
	struct operation *op = &operation;
	int rc = 0;

	reset_service(true);
	service.onoff_request_rv = -3;

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);

	zassert_true(service_has_error(&service),
		     "missing error");
}

static void test_reset_notsup(void)
{
	struct operation operation;
	struct operation *op = &operation;
	int rc = 0;

	reset_service(false);

	zassert_false(service_has_error(&service),
		      "missing error");
	rc = service_reset(&service, NULL);
	zassert_equal(rc, -ENOTSUP,
		      "unexpected reset: %d\n", rc);

	reset_service(true);

	struct onoff_transitions onoff_transitions = *service.onoff.transitions;
	onoff_transitions.reset = NULL;
	replace_service_onoff(&onoff_transitions);

	zassert_false(service_has_error(&service),
		      "missing error");
	rc = service_reset(&service, NULL);
	zassert_equal(rc, -EALREADY,
		      "unexpected reset: %d\n", rc);

	service.onoff_request_rv = -3;

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);

	zassert_true(service_has_error(&service),
		     "missing error");

	rc = service_reset(&service, NULL);
	zassert_equal(rc, -ENOTSUP,
		      "unexpected reset: %d\n", rc);
}

static void test_reset(void)
{
	struct operation operation;
	struct operation *op = &operation;
	int rc = 0;

	reset_service(true);

	zassert_false(service_has_error(&service),
		      "missing error");
	rc = service_reset(&service, NULL);
	zassert_equal(rc, -EALREADY,
		      "unexpected reset: %d\n", rc);

	service.onoff_request_rv = -3;

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);

	zassert_true(service_has_error(&service),
		     "missing error");

	rc = service_reset(&service, NULL);
	zassert_true(rc >= 0,
		     "unexpected reset: %d\n", rc);

	zassert_false(service_has_error(&service),
		      "reset failed");
}

static void test_notifying_reset(void)
{
	struct operation operation;
	struct operation *op = &operation;
	int res;
	int rc = 0;
	struct onoff_client oocli;

	reset_service(true);

	memset(&oocli, 0, sizeof(oocli));
	sys_notify_init_spinwait(&oocli.notify);

	zassert_false(service_has_error(&service),
		      "missing error");
	rc = service_reset(&service, &oocli);
	zassert_equal(rc, -EALREADY,
		      "unexpected reset: %d\n", rc);

	service.onoff_request_rv = -3;

	operation_init_spinwait(&operation);
	rc = service_submit(&service, op, 0);

	zassert_true(service_has_error(&service),
		     "missing error");

	service.async_onoff = true;

	rc = service_reset(&service, &oocli);
	zassert_true(rc >= 0,
		     "unexpected reset: %d\n", rc);

	zassert_true(service_has_error(&service),
		     "missing error");

	rc = sys_notify_fetch_result(&oocli.notify, &res);
	zassert_equal(rc, -EAGAIN,
		      "unexpected fetch async: %d", rc);

	int reset_res = 21;

	service_onoff_notify(reset_res);

	zassert_false(service_has_error(&service),
		      "reset failed");

	rc = sys_notify_fetch_result(&oocli.notify, &res);
	zassert_equal(rc, 0,
		      "unexpected fetch complete: %d", rc);
	zassert_equal(res, reset_res,
		      "unexpected completion: %d", res);
}

void test_main(void)
{
	ztest_test_suite(queued_operation_api,
			 ztest_unit_test(test_notification_spinwait),
			 ztest_unit_test(test_notification_signal),
			 ztest_unit_test(test_notification_callback),
			 ztest_unit_test(test_sync_priority),
			 ztest_unit_test(test_special_priority),
			 ztest_unit_test(test_resubmit_priority),
			 ztest_unit_test(test_missing_validation),
			 ztest_unit_test(test_success_validation),
			 ztest_unit_test(test_failed_validation),
			 ztest_unit_test(test_callback_validation),
			 ztest_unit_test(test_priority_validation),
			 ztest_unit_test(test_async_idle),
			 ztest_unit_test(test_cancel_active),
			 ztest_unit_test(test_cancel_inactive),
			 ztest_unit_test(test_onoff_success),
			 ztest_unit_test(test_onoff_start_sync_failure),
			 ztest_unit_test(test_onoff_start_failure),
			 ztest_unit_test(test_onoff_restart),
			 ztest_unit_test(test_onoff_stop_failure),
			 ztest_unit_test(test_has_error),
			 ztest_unit_test(test_reset_notsup),
			 ztest_unit_test(test_reset),
			 ztest_unit_test(test_notifying_reset)
			 );
	ztest_run_test_suite(queued_operation_api);
}
