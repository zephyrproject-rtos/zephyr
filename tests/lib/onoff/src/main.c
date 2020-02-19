/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/onoff.h>

static struct onoff_client spinwait_cli;

static int callback_res;
static void *callback_ud;
static void callback(struct onoff_service *srv,
		     struct onoff_client *cli,
		     void *ud,
		     int res)
{
	callback_ud = ud;
	callback_res = res;
}

static inline void init_notify_sig(struct onoff_client *cli,
				   struct k_poll_signal *sig)
{
	k_poll_signal_init(sig);
	onoff_client_init_signal(cli, sig);
}

static inline void init_notify_cb(struct onoff_client *cli)
{
	onoff_client_init_callback(cli, callback, NULL);
}

static inline void init_spinwait(struct onoff_client *cli)
{
	onoff_client_init_spinwait(cli);
}

static inline int cli_result(const struct onoff_client *cli)
{
	int result;
	int rc = onoff_client_fetch_result(cli, &result);

	if (rc == 0) {
		rc = result;
	}
	return rc;
}

struct transit_state {
	const char *tag;
	bool async;
	int retval;
	onoff_service_notify_fn notify;
	struct onoff_service *srv;
};

static void reset_transit_state(struct transit_state *tsp)
{
	tsp->async = false;
	tsp->retval = 0;
	tsp->notify = NULL;
	tsp->srv = NULL;
}

static void run_transit(struct onoff_service *srv,
			onoff_service_notify_fn notify,
			struct transit_state *tsp)
{
	if (tsp->async) {
		TC_PRINT("%s async\n", tsp->tag);
		tsp->notify = notify;
		tsp->srv = srv;
	} else {
		TC_PRINT("%s notify %d\n", tsp->tag, tsp->retval);
		notify(srv, tsp->retval);
	}
}

static void notify(struct transit_state *tsp)
{
	TC_PRINT("%s settle %d\n", tsp->tag, tsp->retval);
	tsp->notify(tsp->srv, tsp->retval);
	tsp->notify = NULL;
	tsp->srv = NULL;
}

static struct k_sem isr_sync;
static struct k_timer isr_timer;

static void isr_notify(struct k_timer *timer)
{
	struct transit_state *tsp = k_timer_user_data_get(timer);

	TC_PRINT("ISR NOTIFY %s %d\n", tsp->tag, k_is_in_isr());
	notify(tsp);
	k_sem_give(&isr_sync);
}

struct isr_call_state {
	struct onoff_service *srv;
	struct onoff_client *cli;
	int result;
};

static void isr_request(struct k_timer *timer)
{
	struct isr_call_state *rsp = k_timer_user_data_get(timer);

	rsp->result = onoff_request(rsp->srv, rsp->cli);
	k_sem_give(&isr_sync);
}

static void isr_release(struct k_timer *timer)
{
	struct isr_call_state *rsp = k_timer_user_data_get(timer);

	rsp->result = onoff_release(rsp->srv, rsp->cli);
	k_sem_give(&isr_sync);
}

static void isr_reset(struct k_timer *timer)
{
	struct isr_call_state *rsp = k_timer_user_data_get(timer);

	rsp->result = onoff_service_reset(rsp->srv, rsp->cli);
	k_sem_give(&isr_sync);
}

static struct transit_state start_state = {
	.tag = "start",
};
static void start(struct onoff_service *srv,
		  onoff_service_notify_fn notify)
{
	run_transit(srv, notify, &start_state);
}

static struct transit_state stop_state = {
	.tag = "stop",
};
static void stop(struct onoff_service *srv,
		 onoff_service_notify_fn notify)
{
	run_transit(srv, notify, &stop_state);
}

static struct transit_state reset_state = {
	.tag = "reset",
};
static void reset(struct onoff_service *srv,
		  onoff_service_notify_fn notify)
{
	run_transit(srv, notify, &reset_state);
}

static void clear_transit(void)
{
	callback_res = 0;
	reset_transit_state(&start_state);
	reset_transit_state(&stop_state);
	reset_transit_state(&reset_state);
}

static void test_service_init_validation(void)
{
	int rc;
	struct onoff_service srv;
	const struct onoff_service_transitions null_transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(NULL, NULL, NULL, 0);
	const struct onoff_service_transitions start_transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, NULL, NULL, 0);
	const struct onoff_service_transitions stop_transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(NULL, stop, NULL, 0);
	struct onoff_service_transitions start_stop_transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL, 0);
	const struct onoff_service_transitions all_transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, reset,
						ONOFF_SERVICE_START_SLEEPS);

	clear_transit();

	rc = onoff_service_init(NULL, &null_transitions);
	zassert_equal(rc, -EINVAL,
		      "init null srv %d", rc);

	rc = onoff_service_init(&srv, &null_transitions);
	zassert_equal(rc, -EINVAL,
		      "init null transit %d", rc);

	rc = onoff_service_init(&srv, &start_transitions);
	zassert_equal(rc, -EINVAL,
		      "init null stop %d", rc);

	rc = onoff_service_init(&srv, &stop_transitions);
	zassert_equal(rc, -EINVAL,
		      "init null start %d", rc);

	start_stop_transitions.flags |= ONOFF_SERVICE_INTERNAL_BASE;
	rc = onoff_service_init(&srv, &start_stop_transitions);
	zassert_equal(rc, -EINVAL,
		      "init bad flags %d", rc);

	memset(&srv, 0xA5, sizeof(srv));
	zassert_false(sys_slist_is_empty(&srv.clients),
		      "slist empty");

	rc = onoff_service_init(&srv, &all_transitions);
	zassert_equal(rc, 0,
		      "init good %d", rc);
	zassert_equal(srv.transitions->start, start,
		      "init start mismatch");
	zassert_equal(srv.transitions->stop, stop,
		      "init stop mismatch");
	zassert_equal(srv.transitions->reset, reset,
		      "init reset mismatch");
	zassert_equal(srv.flags, ONOFF_SERVICE_START_SLEEPS,
		      "init flags mismatch");
	zassert_equal(srv.refs, 0,
		      "init refs mismatch");
	zassert_true(sys_slist_is_empty(&srv.clients),
		     "init slist empty");
}

static void test_client_init_validation(void)
{
	struct onoff_client cli;

	clear_transit();

	memset(&cli, 0xA5, sizeof(cli));
	onoff_client_init_spinwait(&cli);
	zassert_equal(z_snode_next_peek(&cli.node), NULL,
		      "cli node mismatch");
	zassert_equal(cli.notify.flags, SYS_NOTIFY_METHOD_SPINWAIT,
		      "cli spinwait flags");

	struct k_poll_signal sig;

	memset(&cli, 0xA5, sizeof(cli));
	onoff_client_init_signal(&cli, &sig);
	zassert_equal(z_snode_next_peek(&cli.node), NULL,
		      "cli signal node");
	zassert_equal(cli.notify.flags, SYS_NOTIFY_METHOD_SIGNAL,
		      "cli signal flags");
	zassert_equal(cli.notify.method.signal, &sig,
		      "cli signal async");

	memset(&cli, 0xA5, sizeof(cli));
	onoff_client_init_callback(&cli, callback, &sig);
	zassert_equal(z_snode_next_peek(&cli.node), NULL,
		      "cli callback node");
	zassert_equal(cli.notify.flags, SYS_NOTIFY_METHOD_CALLBACK,
		      "cli callback flags");
	zassert_equal(cli.notify.method.callback, callback,
		      "cli callback handler");
	zassert_equal(cli.user_data, &sig,
		      "cli callback user_data");
}

static void test_validate_args(void)
{
	int rc;
	struct onoff_service srv;
	struct k_poll_signal sig;
	struct onoff_client cli;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL, 0);

	clear_transit();

	/* The internal validate_args is invoked from request,
	 * release, and reset; test it through the request API.
	 */

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	rc = onoff_request(NULL, NULL);
	zassert_equal(rc, -EINVAL,
		      "validate req null srv");

	rc = onoff_release(NULL, NULL);
	zassert_equal(rc, -EINVAL,
		      "validate rel null srv");

	rc = onoff_release(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "validate rel null cli");

	rc = onoff_request(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "validate req null cli");

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_true(rc > 0,
		     "trigger to on");

	memset(&cli, 0xA3, sizeof(cli));
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EINVAL,
		      "validate req cli flags");

	init_spinwait(&cli);
	cli.notify.flags = SYS_NOTIFY_METHOD_COMPLETED;
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EINVAL,
		      "validate req cli mode");

	init_notify_sig(&cli, &sig);
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, 0,
		      "validate req cli signal: %d", rc);
	init_notify_sig(&cli, &sig);
	cli.notify.method.signal = NULL;
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EINVAL,
		      "validate req cli signal null");

	init_notify_cb(&cli);
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, 0,
		      "validate req cli callback");

	init_notify_cb(&cli);
	cli.notify.method.callback = NULL;
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EINVAL,
		      "validate req cli callback null");

	memset(&cli, 0x3C, sizeof(cli)); /* makes flags invalid */
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EINVAL,
		      "validate req cli notify mode");
}

static void test_reset(void)
{
	int rc;
	struct onoff_service srv;
	struct k_poll_signal sig;
	struct onoff_client cli;
	unsigned int signalled = 0;
	int result = 0;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL, 0);
	struct onoff_service_transitions transitions_with_reset =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, reset, 0);

	clear_transit();

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");
	rc = onoff_service_reset(&srv, &cli);
	zassert_equal(rc, -ENOTSUP,
		      "reset: %d", rc);

	rc = onoff_service_init(&srv, &transitions_with_reset);
	zassert_equal(rc, 0,
		      "service init");

	rc = onoff_service_reset(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "rst no cli");

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_true(rc > 0,
		     "req ok");
	zassert_equal(srv.refs, 1U,
		      "reset req refs: %u", srv.refs);


	zassert_false(onoff_service_has_error(&srv),
		      "has error");
	reset_state.retval = 57;
	init_notify_sig(&cli, &sig);
	rc = onoff_service_reset(&srv, &cli);
	zassert_equal(rc, -EALREADY,
		      "reset: %d", rc);

	stop_state.retval = -23;
	init_notify_sig(&cli, &sig);
	rc = onoff_release(&srv, &cli);
	zassert_equal(rc, 2,
		      "rel trigger: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "reset req refs: %u", srv.refs);
	zassert_true(onoff_service_has_error(&srv),
		     "has error");
	zassert_equal(cli_result(&cli), stop_state.retval,
		      "cli result");
	signalled = 0;
	result = -1;
	k_poll_signal_check(&sig, &signalled, &result);
	zassert_true(signalled != 0,
		     "signalled");
	zassert_equal(result, stop_state.retval,
		      "result");
	k_poll_signal_reset(&sig);

	reset_state.retval = -59;
	init_notify_sig(&cli, &sig);
	rc = onoff_service_reset(&srv, &cli);
	zassert_equal(rc, 0U,
		      "reset: %d", rc);
	zassert_equal(cli_result(&cli), reset_state.retval,
		      "reset result");
	zassert_equal(srv.refs, 0U,
		      "reset req refs: %u", srv.refs);
	zassert_true(onoff_service_has_error(&srv),
		     "has error");

	reset_state.retval = 62;
	init_notify_sig(&cli, &sig);
	rc = onoff_service_reset(&srv, &cli);
	zassert_equal(rc, 0U,
		      "reset: %d", rc);
	zassert_equal(cli_result(&cli), reset_state.retval,
		      "reset result");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	signalled = 0;
	result = -1;
	k_poll_signal_check(&sig, &signalled, &result);
	zassert_true(signalled != 0,
		     "signalled");
	zassert_equal(result, reset_state.retval,
		      "result");

	zassert_equal(srv.refs, 0U,
		      "reset req refs: %u", srv.refs);
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	transitions_with_reset.flags |= ONOFF_SERVICE_RESET_SLEEPS;
	rc = onoff_service_init(&srv, &transitions_with_reset);
	zassert_equal(rc, 0,
		      "service init");
	start_state.retval = -23;
	zassert_false(onoff_service_has_error(&srv),
		      "has error");
	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_true(onoff_service_has_error(&srv),
		     "has error");

	struct isr_call_state isr_state = {
		.srv = &srv,
		.cli = &spinwait_cli,
	};
	struct k_timer timer;

	init_spinwait(&spinwait_cli);
	k_timer_init(&timer, isr_reset, NULL);
	k_timer_user_data_set(&timer, &isr_state);

	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
	rc = k_sem_take(&isr_sync, K_MSEC(10));
	zassert_equal(rc, 0,
		      "isr sync");

	zassert_equal(isr_state.result, -EWOULDBLOCK,
		      "isr reset");
	zassert_equal(cli_result(&spinwait_cli), -EAGAIN,
		      "is reset result");
}

static void test_request(void)
{
	int rc;
	struct onoff_service srv;
	struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, reset, 0);

	clear_transit();

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_true(rc >= 0,
		     "reset req: %d", rc);
	zassert_equal(srv.refs, 1U,
		      "reset req refs: %u", srv.refs);
	zassert_equal(cli_result(&spinwait_cli), 0,
		      "reset req result: %d", cli_result(&spinwait_cli));

	/* Can't reset when no error present. */
	init_spinwait(&spinwait_cli);
	rc = onoff_service_reset(&srv, &spinwait_cli);
	zassert_equal(rc, -EALREADY,
		      "reset spin client");

	/* Reference overflow produces -EAGAIN */
	u32_t refs = srv.refs;

	srv.refs = UINT16_MAX;
	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, -EAGAIN,
		      "reset req overflow: %d", rc);
	srv.refs = refs;

	/* Force an error. */
	stop_state.retval = -32;
	init_spinwait(&spinwait_cli);
	rc = onoff_release(&srv, &spinwait_cli);
	zassert_equal(rc, 2,
		      "error release");
	zassert_equal(cli_result(&spinwait_cli), stop_state.retval,
		      "error retval");
	zassert_true(onoff_service_has_error(&srv),
		     "has error");

	/* Can't request when error present. */
	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, -EIO,
		      "req with error");

	/* Can't release when error present. */
	init_spinwait(&spinwait_cli);
	rc = onoff_release(&srv, &spinwait_cli);
	zassert_equal(rc, -EIO,
		      "rel with error");

	struct k_poll_signal sig;
	struct onoff_client cli;

	/* Clear the error */
	init_notify_sig(&cli, &sig);
	rc = onoff_service_reset(&srv, &cli);
	zassert_equal(rc, 0,
		      "reset");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	/* Error on start */
	start_state.retval = -12;
	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, 2,
		      "req with error");
	zassert_equal(cli_result(&spinwait_cli), start_state.retval,
		      "req with error");
	zassert_true(onoff_service_has_error(&srv),
		     "has error");

	/* Clear the error */
	init_spinwait(&spinwait_cli);
	rc = onoff_service_reset(&srv, &spinwait_cli);
	zassert_equal(rc, 0,
		      "reset");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	/* Diagnose a no-wait delayed start */
	transitions.flags |= ONOFF_SERVICE_START_SLEEPS;
	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");
	start_state.async = true;
	start_state.retval = 12;

	struct isr_call_state isr_state = {
		.srv = &srv,
		.cli = &spinwait_cli,
	};
	struct k_timer timer;

	init_spinwait(&spinwait_cli);
	k_timer_init(&timer, isr_request, NULL);
	k_timer_user_data_set(&timer, &isr_state);

	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
	rc = k_sem_take(&isr_sync, K_MSEC(10));
	zassert_equal(rc, 0,
		      "isr sync");

	zassert_equal(isr_state.result, -EWOULDBLOCK,
		      "isr request");
	zassert_equal(cli_result(&spinwait_cli), -EAGAIN,
		      "isr request result");
}

static void test_sync(void)
{
	int rc;
	struct onoff_service srv;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, reset, 0);

	clear_transit();

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	/* WHITEBOX: request that triggers on returns positive */
	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, 2,    /* WHITEBOX starting request */
		      "req ok");
	zassert_equal(srv.refs, 1U,
		      "reset req refs: %u", srv.refs);

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, 0,    /* WHITEBOX on request */
		      "req ok");
	zassert_equal(srv.refs, 2U,
		      "reset req refs: %u", srv.refs);

	init_spinwait(&spinwait_cli);
	rc = onoff_release(&srv, &spinwait_cli);
	zassert_equal(rc, 1,    /* WHITEBOX non-stopping release */
		      "rel ok");
	zassert_equal(srv.refs, 1U,
		      "reset rel refs: %u", srv.refs);

	init_spinwait(&spinwait_cli);
	rc = onoff_release(&srv, &spinwait_cli);
	zassert_equal(rc, 2,    /* WHITEBOX stopping release*/
		      "rel ok: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "reset rel refs: %u", srv.refs);

	init_spinwait(&spinwait_cli);
	rc = onoff_release(&srv, &spinwait_cli);
	zassert_equal(rc, -EALREADY,
		      "rel noent");
}

static void test_async(void)
{
	int rc;
	struct onoff_service srv;
	struct k_poll_signal sig[2];
	struct onoff_client cli[2];
	unsigned int signalled = 0;
	int result = 0;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, reset,
			ONOFF_SERVICE_START_SLEEPS | ONOFF_SERVICE_STOP_SLEEPS);

	clear_transit();
	start_state.async = true;
	start_state.retval = 23;
	stop_state.async = true;
	stop_state.retval = 17;

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	/* WHITEBOX: request that triggers on returns positive */
	init_notify_sig(&cli[0], &sig[0]);
	rc = onoff_request(&srv, &cli[0]);
	zassert_equal(rc, 2,    /* WHITEBOX starting request */
		      "req ok");
	k_poll_signal_check(&sig[0], &signalled, &result);
	zassert_equal((bool)signalled, false,
		      "cli signalled");
	zassert_equal(srv.refs, 0U,
		      "reset req refs: %u", srv.refs);


	/* Non-initial request from ISR is OK */
	struct onoff_client isrcli;
	struct isr_call_state isr_state = {
		.srv = &srv,
		.cli = &isrcli,
	};
	struct k_timer timer;

	init_spinwait(&isrcli);
	k_timer_init(&timer, isr_request, NULL);
	k_timer_user_data_set(&timer, &isr_state);

	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
	rc = k_sem_take(&isr_sync, K_MSEC(10));
	zassert_equal(rc, 0,
		      "isr sync");

	zassert_equal(isr_state.result, 1, /* WHITEBOX pending request */
		      "isr request: %d", isr_state.result);
	zassert_equal(cli_result(&isrcli), -EAGAIN,
		      "isr request result");

	/* Off while on pending is not supported */
	init_notify_sig(&cli[1], &sig[1]);
	rc = onoff_release(&srv, &cli[1]);
	zassert_equal(rc, -EBUSY,
		      "rel in to-on");

	/* Second request is delayed for first. */
	init_notify_sig(&cli[1], &sig[1]);
	rc = onoff_request(&srv, &cli[1]);
	zassert_equal(rc, 1,    /* WHITEBOX pending request */
		      "req ok");
	k_poll_signal_check(&sig[1], &signalled, &result);
	zassert_equal((bool)signalled, false,
		      "cli signalled");
	zassert_equal(srv.refs, 0U,
		      "reset req refs: %u", srv.refs);

	/* Complete the transition. */
	notify(&start_state);
	k_poll_signal_check(&sig[0], &signalled, &result);
	k_poll_signal_reset(&sig[0]);
	zassert_equal((bool)signalled, true,
		      "cli signalled");
	zassert_equal(result, start_state.retval,
		      "cli result");
	zassert_equal(cli_result(&isrcli), start_state.retval,
		      "isrcli result");
	k_poll_signal_check(&sig[1], &signalled, &result);
	k_poll_signal_reset(&sig[1]);
	zassert_equal((bool)signalled, true,
		      "cli2 signalled");
	zassert_equal(result, start_state.retval,
		      "cli2 result");
	zassert_equal(srv.refs, 3U,
		      "reset req refs: %u", srv.refs);

	/* Non-final release decrements refs and completes. */
	init_notify_sig(&cli[0], &sig[0]);
	rc = onoff_release(&srv, &cli[0]);
	zassert_equal(rc, 1,    /* WHITEBOX non-stopping release */
		      "rel ok");
	zassert_equal(srv.refs, 2U,
		      "reset rel refs: %u", srv.refs);
	k_poll_signal_check(&sig[0], &signalled, &result);
	k_poll_signal_reset(&sig[0]);
	zassert_equal((bool)signalled, true,
		      "cli signalled");
	zassert_equal(result, 0,
		      "cli result");

	/* Non-final release from ISR is OK */
	init_spinwait(&isrcli);
	k_timer_init(&timer, isr_release, NULL);
	k_timer_user_data_set(&timer, &isr_state);

	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
	rc = k_sem_take(&isr_sync, K_MSEC(10));
	zassert_equal(rc, 0,
		      "isr sync");

	zassert_equal(isr_state.result, 1, /* WHITEBOX pending request */
		      "isr release: %d", isr_state.result);
	zassert_equal(cli_result(&isrcli), 0,
		      "isr release result");
	zassert_equal(srv.refs, 1U,
		      "reset rel refs: %u", srv.refs);

	/* Final release cannot be from ISR */

	init_spinwait(&isrcli);
	k_timer_start(&timer, K_MSEC(1), K_NO_WAIT);
	rc = k_sem_take(&isr_sync, K_MSEC(10));
	zassert_equal(rc, 0,
		      "isr sync");

	zassert_equal(isr_state.result, -EWOULDBLOCK,
		      "isr release");
	zassert_equal(cli_result(&isrcli), -EAGAIN,
		      "is release result");

	/* Final async release holds until notify */
	init_notify_sig(&cli[1], &sig[1]);
	rc = onoff_release(&srv, &cli[1]);
	zassert_equal(rc, 2,    /* WHITEBOX stopping release */
		      "rel ok: %d", rc);
	zassert_equal(srv.refs, 1U,
		      "reset rel refs: %u", srv.refs);

	/* Redundant release in to-off */
	init_notify_sig(&cli[0], &sig[0]);
	rc = onoff_release(&srv, &cli[0]);
	zassert_equal(rc, -EALREADY,
		      "rel to-off: %d", rc);
	zassert_equal(srv.refs, 1U,
		      "reset rel refs: %u", srv.refs);
	k_poll_signal_check(&sig[0], &signalled, &result);
	zassert_equal((bool)signalled, false,
		      "cli signalled");

	/* Request when turning off is queued */
	init_notify_sig(&cli[0], &sig[0]);
	rc = onoff_request(&srv, &cli[0]);
	zassert_equal(rc, 3,    /* WHITEBOX stopping request */
		      "req in to-off");

	/* Finalize release, queues start */
	zassert_true(start_state.notify == NULL,
		     "start not invoked");
	notify(&stop_state);
	zassert_false(start_state.notify == NULL,
		      "start invoked");
	zassert_equal(srv.refs, 0U,
		      "reset rel refs: %u", srv.refs);
	k_poll_signal_check(&sig[1], &signalled, &result);
	k_poll_signal_reset(&sig[1]);
	zassert_equal((bool)signalled, true,
		      "cli signalled");
	zassert_equal(result, stop_state.retval,
		      "cli result");

	/* Release when starting is an error */
	init_notify_sig(&cli[0], &sig[0]);
	rc = onoff_release(&srv, &cli[0]);
	zassert_equal(rc, -EBUSY,
		      "rel to-off: %d", rc);

	/* Finalize queued start, gets us to on */
	cli[0].notify.result = 1 + start_state.retval;
	zassert_equal(cli_result(&cli[0]), -EAGAIN,
		      "fetch failed");
	zassert_false(start_state.notify == NULL,
		      "start invoked");
	notify(&start_state);
	zassert_equal(cli_result(&cli[0]), start_state.retval,
		      "start notified");
	zassert_equal(srv.refs, 1U,
		      "reset rel refs: %u", srv.refs);
}

static void test_half_sync(void)
{
	int rc;
	struct onoff_service srv;
	struct k_poll_signal sig;
	struct onoff_client cli;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL,
						ONOFF_SERVICE_STOP_SLEEPS);

	clear_transit();
	start_state.retval = 23;
	stop_state.async = true;
	stop_state.retval = 17;

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	/* Test that a synchronous start delayed by a pending
	 * asynchronous stop is accepted.
	 */
	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, 2,
		      "req0");
	zassert_equal(srv.refs, 1U,
		      "active");
	zassert_equal(cli_result(&spinwait_cli), start_state.retval,
		      "request");

	zassert_true(stop_state.notify == NULL,
		     "not stopping");
	init_notify_sig(&cli, &sig);
	rc = onoff_release(&srv, &cli);
	zassert_equal(rc, 2,
		      "rel0");
	zassert_equal(srv.refs, 1U,
		      "active");
	zassert_false(stop_state.notify == NULL,
		      "stop pending");

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, 3,    /* WHITEBOX start delayed for stop */
		      "restart");

	zassert_equal(cli_result(&cli), -EAGAIN,
		      "stop incomplete");
	zassert_equal(cli_result(&spinwait_cli), -EAGAIN,
		      "restart incomplete");
	notify(&stop_state);
	zassert_equal(cli_result(&cli), stop_state.retval,
		      "stop complete");
	zassert_equal(cli_result(&spinwait_cli), start_state.retval,
		      "restart complete");
}

static void test_cancel_request_waits(void)
{
	int rc;
	struct onoff_service srv;
	struct k_poll_signal sig;
	struct onoff_client cli;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL,
			ONOFF_SERVICE_START_SLEEPS | ONOFF_SERVICE_STOP_SLEEPS);

	clear_transit();
	start_state.async = true;
	start_state.retval = 14;
	stop_state.async = true;
	stop_state.retval = 31;

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	init_notify_sig(&cli, &sig);
	rc = onoff_request(&srv, &cli);
	zassert_true(rc > 0,
		     "request pending");
	zassert_false(start_state.notify == NULL,
		      "start pending");
	zassert_equal(cli_result(&cli), -EAGAIN,
		      "start pending");

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, 1, /* WHITEBOX secondary request */
		      "start2 pending");
	zassert_equal(cli_result(&spinwait_cli), -EAGAIN,
		      "start2 pending");

	/* Allowed to cancel in-progress start if doing so leaves
	 * something to receive the start completion.
	 */
	rc = onoff_cancel(&srv, &cli);
	zassert_equal(rc, 0,
		      "cancel failed: %d", rc);
	zassert_equal(cli_result(&cli), -ECANCELED,
		      "cancel notified");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	/* Not allowed to cancel the last pending start.
	 */
	rc = onoff_cancel(&srv, &spinwait_cli);
	zassert_equal(rc, -EWOULDBLOCK,
		      "last cancel", rc);
	zassert_false(onoff_service_has_error(&srv),
		      "has error");
	zassert_equal(cli_result(&spinwait_cli), -EAGAIN,
		      "last request");

	notify(&start_state);
	zassert_equal(cli_result(&spinwait_cli), start_state.retval,
		      "last request");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");


	/* Issue a stop, then confirm that you can request and cancel
	 * a restart.
	 */
	init_spinwait(&cli);
	rc = onoff_release(&srv, &cli);
	zassert_equal(rc, 2, /* WHITEBOX stop pending */
		      "stop pending, %d", rc);
	zassert_equal(cli_result(&cli), -EAGAIN,
		      "stop pending");

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_equal(rc, 3, /* WHITEBOX restart pending */
		      "restart pending");

	rc = onoff_cancel(&srv, &spinwait_cli);
	zassert_equal(rc, 0,
		      "restart cancel");
	zassert_equal(cli_result(&spinwait_cli), -ECANCELED,
		      "restart cancel");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	zassert_equal(cli_result(&cli), -EAGAIN,
		      "stop pending");

	notify(&stop_state);
	zassert_equal(cli_result(&cli), stop_state.retval,
		      "released");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");
}

static void test_cancel_request_ok(void)
{
	int rc;
	struct onoff_service srv;
	struct k_poll_signal sig;
	struct onoff_client cli;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL,
						ONOFF_SERVICE_START_SLEEPS);

	clear_transit();
	start_state.async = true;
	start_state.retval = 14;
	stop_state.retval = 31;

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	init_notify_sig(&cli, &sig);
	rc = onoff_request(&srv, &cli);
	zassert_true(rc > 0,
		     "request pending");
	zassert_false(start_state.notify == NULL,
		      "start pending");

	/* You can't cancel the last start request */
	rc = onoff_cancel(&srv, &cli);
	zassert_equal(rc, -EWOULDBLOCK,
		      "cancel");
	zassert_equal(srv.refs, 0,
		      "refs empty");

	notify(&start_state);
	zassert_equal(srv.refs, 1,
		      "refs");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");
	zassert_equal(cli_result(&cli), start_state.retval,
		      "cancel notified");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	/* You can "cancel" an request that isn't active */
	init_spinwait(&cli);
	rc = onoff_cancel(&srv, &cli);
	zassert_equal(rc, -EALREADY,
		      "unregistered");

	/* Error if cancel params invalid */
	rc = onoff_cancel(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "invalid");
}

static void test_blocked_restart(void)
{
	int rc;
	struct onoff_service srv;
	unsigned int signalled = 0;
	int result;
	struct k_poll_signal sig[2];
	struct onoff_client cli[2];
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL,
			ONOFF_SERVICE_START_SLEEPS | ONOFF_SERVICE_STOP_SLEEPS);

	clear_transit();
	start_state.async = true;
	start_state.retval = 14;
	stop_state.async = true;
	stop_state.retval = 31;

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	init_notify_sig(&cli[0], &sig[0]);
	rc = onoff_request(&srv, &cli[0]);
	zassert_true(rc > 0,
		     "started");
	zassert_false(start_state.notify == NULL,
		      "start pending");
	notify(&start_state);

	result = -start_state.retval;
	k_poll_signal_check(&sig[0], &signalled, &result);
	zassert_true(signalled != 0,
		     "signalled");
	zassert_equal(result, start_state.retval,
		      "result");
	k_poll_signal_reset(&sig[0]);

	start_state.async = true;
	init_notify_sig(&cli[0], &sig[0]);
	rc = onoff_release(&srv, &cli[0]);
	zassert_true(rc > 0,
		     "stop initiated");
	zassert_false(stop_state.notify == NULL,
		      "stop pending");
	init_notify_sig(&cli[1], &sig[1]);
	rc = onoff_request(&srv, &cli[1]);
	zassert_true(rc > 0,
		     "start pending");

	result = start_state.retval + stop_state.retval;
	k_poll_signal_check(&sig[0], &signalled, &result);
	zassert_true(signalled == 0,
		     "stop signalled");
	k_poll_signal_check(&sig[1], &signalled, &result);
	zassert_true(signalled == 0,
		     "restart signalled");

	k_timer_user_data_set(&isr_timer, &stop_state);
	k_timer_start(&isr_timer, K_MSEC(1), K_NO_WAIT);
	rc = k_sem_take(&isr_sync, K_MSEC(10));
	zassert_equal(rc, 0,
		      "isr sync");

	/* Fail-to-restart is not an error */
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	k_poll_signal_check(&sig[0], &signalled, &result);
	zassert_false(signalled == 0,
		      "stop pending");
	zassert_equal(result, stop_state.retval,
		      "stop succeeded");

	k_poll_signal_check(&sig[1], &signalled, &result);
	zassert_false(signalled == 0,
		      "restart pending");
	zassert_equal(result, -EWOULDBLOCK,
		      "restart failed");
}

static void test_cancel_release(void)
{
	int rc;
	struct onoff_service srv;
	const struct onoff_service_transitions transitions =
		ONOFF_SERVICE_TRANSITIONS_INITIALIZER(start, stop, NULL,
			ONOFF_SERVICE_STOP_SLEEPS);

	clear_transit();
	start_state.retval = 16;
	stop_state.async = true;
	stop_state.retval = 94;

	rc = onoff_service_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	init_spinwait(&spinwait_cli);
	rc = onoff_request(&srv, &spinwait_cli);
	zassert_true(rc > 0,
		     "request done");
	zassert_equal(cli_result(&spinwait_cli), start_state.retval,
		      "started");

	init_spinwait(&spinwait_cli);
	rc = onoff_release(&srv, &spinwait_cli);
	zassert_true(rc > 0,
		     "release pending");
	zassert_false(stop_state.notify == NULL,
		      "release pending");
	zassert_equal(cli_result(&spinwait_cli), -EAGAIN,
		      "release pending");

	/* You can't cancel a stop request. */
	rc = onoff_cancel(&srv, &spinwait_cli);
	zassert_equal(rc, -EWOULDBLOCK,
		      "cancel succeeded");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");

	notify(&stop_state);
	zassert_equal(cli_result(&spinwait_cli), stop_state.retval,
		      "release pending");
	zassert_false(onoff_service_has_error(&srv),
		      "has error");
}

void test_main(void)
{
	k_sem_init(&isr_sync, 0, 1);
	k_timer_init(&isr_timer, isr_notify, NULL);

	ztest_test_suite(onoff_api,
			 ztest_unit_test(test_service_init_validation),
			 ztest_unit_test(test_client_init_validation),
			 ztest_unit_test(test_validate_args),
			 ztest_unit_test(test_reset),
			 ztest_unit_test(test_request),
			 ztest_unit_test(test_sync),
			 ztest_unit_test(test_async),
			 ztest_unit_test(test_half_sync),
			 ztest_unit_test(test_cancel_request_waits),
			 ztest_unit_test(test_cancel_request_ok),
			 ztest_unit_test(test_blocked_restart),
			 ztest_unit_test(test_cancel_release));
	ztest_run_test_suite(onoff_api);
}
