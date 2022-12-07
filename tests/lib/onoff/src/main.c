/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/onoff.h>

static struct onoff_client cli;
struct onoff_transitions transitions;
static struct onoff_manager srv;
static struct onoff_monitor mon;

struct transition_record {
	uint32_t state;
	int res;
};
static struct transition_record trans[32];
static size_t ntrans;

static void trans_callback(struct onoff_manager *mgr,
			  struct onoff_monitor *mon,
			  uint32_t state,
			  int res)
{
	if (ntrans < ARRAY_SIZE(trans)) {
		trans[ntrans++] = (struct transition_record){
			.state = state,
			.res = res,
		};
	}
}

static void check_trans(uint32_t idx,
		       uint32_t state,
		       int res,
		       const char *tag)
{
	zassert_true(idx < ntrans,
		     "trans idx %u high: %s", idx, tag);

	const struct transition_record *xp = trans + idx;

	zassert_equal(xp->state, state,
		      "trans[%u] state %x != %x: %s",
		      idx, xp->state, state, tag);
	zassert_equal(xp->res, res,
		      "trans[%u] res %d != %d: %s",
		      idx, xp->res, res, tag);
}

static struct onoff_manager *callback_srv;
static struct onoff_client *callback_cli;
static uint32_t callback_state;
static int callback_res;
static onoff_client_callback callback_fn;

static void callback(struct onoff_manager *srv,
		     struct onoff_client *cli,
		     uint32_t state,
		     int res)
{
	onoff_client_callback cb = callback_fn;

	callback_srv = srv;
	callback_cli = cli;
	callback_state = state;
	callback_res = res;
	callback_fn = NULL;

	if (cb != NULL) {
		cb(srv, cli, state, res);
	}
}

static void check_callback(uint32_t state,
			   int res,
			   const char *tag)
{
	zassert_equal(callback_state, state,
		      "callback state %x != %x: %s",
		      callback_state, state, tag);
	zassert_equal(callback_res, res,
		      "callback res %d != %d: %s",
		      callback_res, res, tag);
}

static inline int cli_result(const struct onoff_client *cp)
{
	int result;
	int rc = sys_notify_fetch_result(&cp->notify, &result);

	if (rc == 0) {
		rc = result;
	}
	return rc;
}

static void check_result(int res,
			 const char *tag)
{
	zassert_equal(cli_result(&cli), res,
		      "cli res %d != %d: %s",
		      cli_result(&cli), res, tag);
}

struct transit_state {
	const char *tag;
	bool async;
	int retval;
	onoff_notify_fn notify;
	struct onoff_manager *srv;
};

static void reset_transit_state(struct transit_state *tsp)
{
	tsp->async = false;
	tsp->retval = 0;
	tsp->notify = NULL;
	tsp->srv = NULL;
}

static void run_transit(struct onoff_manager *srv,
			onoff_notify_fn notify,
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
	TC_PRINT("%s settle %d %p\n", tsp->tag, tsp->retval, tsp->notify);
	tsp->notify(tsp->srv, tsp->retval);
	tsp->notify = NULL;
	tsp->srv = NULL;
}

static struct transit_state start_state = {
	.tag = "start",
};
static void start(struct onoff_manager *srv,
		  onoff_notify_fn notify)
{
	run_transit(srv, notify, &start_state);
}

static struct transit_state stop_state = {
	.tag = "stop",
};
static void stop(struct onoff_manager *srv,
		 onoff_notify_fn notify)
{
	run_transit(srv, notify, &stop_state);
}

static struct transit_state reset_state = {
	.tag = "reset",
};
static void reset(struct onoff_manager *srv,
		  onoff_notify_fn notify)
{
	run_transit(srv, notify, &reset_state);
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
	struct onoff_manager *srv;
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

	rsp->result = onoff_release(rsp->srv);
	k_sem_give(&isr_sync);
}

static void isr_reset(struct k_timer *timer)
{
	struct isr_call_state *rsp = k_timer_user_data_get(timer);

	rsp->result = onoff_reset(rsp->srv, rsp->cli);
	k_sem_give(&isr_sync);
}

static void reset_cli(void)
{
	cli = (struct onoff_client){};
	sys_notify_init_callback(&cli.notify, callback);
}

static void reset_callback(void)
{
	callback_state = -1;
	callback_res = 0;
	callback_fn = NULL;
}

static void setup_test(void)
{
	int rc;

	reset_callback();
	reset_transit_state(&start_state);
	reset_transit_state(&stop_state);
	reset_transit_state(&reset_state);
	ntrans = 0;

	transitions = (struct onoff_transitions)
		      ONOFF_TRANSITIONS_INITIALIZER(start, stop, reset);
	rc = onoff_manager_init(&srv, &transitions);
	zassert_equal(rc, 0,
		      "service init");

	mon = (struct onoff_monitor){
		.callback = trans_callback,
	};
	rc = onoff_monitor_register(&srv, &mon);
	zassert_equal(rc, 0,
		      "mon reg");

	reset_cli();
}

static void setup_error(void)
{
	int rc;

	setup_test();
	start_state.retval = -1;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req 0 0");
	check_result(start_state.retval,
		     "err req");
	zassert_true(onoff_has_error(&srv),
		     "has_err");

	reset_cli();
}

ZTEST(onoff_api, test_manager_init)
{
	int rc;
	struct onoff_transitions xit = {};

	setup_test();

	rc = onoff_manager_init(NULL, NULL);
	zassert_equal(rc, -EINVAL,
		      "init 0 0");
	rc = onoff_manager_init(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "init srv 0");
	rc = onoff_manager_init(NULL, &transitions);
	zassert_equal(rc, -EINVAL,
		      "init 0 xit");
	rc = onoff_manager_init(&srv, &xit);
	zassert_equal(rc, -EINVAL,
		      "init 0 xit-start");

	xit.start = start;
	rc = onoff_manager_init(&srv, &xit);
	zassert_equal(rc, -EINVAL,
		      "init srv xit-stop");

	xit.stop = stop;
	rc = onoff_manager_init(&srv, &xit);
	zassert_equal(rc, 0,
		      "init srv xit ok");
}

ZTEST(onoff_api, test_mon_reg)
{
	static struct onoff_monitor mon = {};

	setup_test();

	/* Verify parameter validation */

	zassert_equal(onoff_monitor_register(NULL, NULL), -EINVAL,
		      "mon reg 0 0");
	zassert_equal(onoff_monitor_register(&srv, NULL), -EINVAL,
		      "mon reg srv 0");
	zassert_equal(onoff_monitor_register(NULL, &mon), -EINVAL,
		      "mon reg 0 mon");
	zassert_equal(onoff_monitor_register(&srv, &mon), -EINVAL,
		      "mon reg srv mon(!cb)");
}

ZTEST(onoff_api, test_mon_unreg)
{
	setup_test();

	/* Verify parameter validation */

	zassert_equal(onoff_monitor_unregister(NULL, NULL), -EINVAL,
		      "mon unreg 0 0");
	zassert_equal(onoff_monitor_unregister(&srv, NULL), -EINVAL,
		      "mon unreg srv 0");
	zassert_equal(onoff_monitor_unregister(NULL, &mon), -EINVAL,
		      "mon unreg 0 mon");
	zassert_equal(onoff_monitor_unregister(&srv, &mon), 0,
		      "mon unreg 0 mon");
	zassert_equal(onoff_monitor_unregister(&srv, &mon), -EINVAL,
		      "mon unreg 0 mon");
}

ZTEST(onoff_api, test_request)
{
	struct onoff_client cli2 = {};
	int rc;

	setup_test();

	rc = onoff_request(NULL, NULL);
	zassert_equal(rc, -EINVAL,
		      "req 0 0");
	rc = onoff_request(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "req srv 0");
	rc = onoff_request(NULL, &cli);
	zassert_equal(rc, -EINVAL,
		      "req 0 cli");

	rc = onoff_request(&srv, &cli2);
	zassert_equal(rc, -EINVAL,
		      "req srv cli-uninit");

	cli.notify.flags |= BIT(ONOFF_CLIENT_EXTENSION_POS);
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EINVAL,
		      "req srv cli-flags");

	cli.notify.flags &= ~BIT(ONOFF_CLIENT_EXTENSION_POS);
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, 0,
		      "req srv cli ok");

	reset_cli();
	srv.refs = -1;
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EAGAIN,
		      "req srv cli ofl");

}

ZTEST(onoff_api, test_basic_sync)
{
	int rc;

	/* Verify synchronous request and release behavior. */

	setup_test();
	start_state.retval = 16;
	stop_state.retval = 23;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req: %d", rc);
	zassert_equal(srv.refs, 1U,
		      "req refs: %u", srv.refs);
	check_result(start_state.retval, "req");
	zassert_equal(callback_srv, &srv,
		      "callback wrong srv");
	zassert_equal(callback_cli, &cli,
		      "callback wrong cli");
	check_callback(ONOFF_STATE_ON, start_state.retval,
		       "req");
	zassert_equal(ntrans, 2U,
		      "req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");

	rc = onoff_release(&srv);
	zassert_equal(rc, ONOFF_STATE_ON,
		      "rel: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "rel refs: %u", srv.refs);
	zassert_equal(ntrans, 4U,
		      "rel trans");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");

	rc = onoff_release(&srv);
	zassert_equal(rc, -ENOTSUP,
		      "re-rel: %d", rc);
}

ZTEST(onoff_api, test_basic_async)
{
	int rc;

	/* Verify asynchronous request and release behavior. */

	setup_test();
	start_state.async = true;
	start_state.retval = 51;
	stop_state.async = true;
	stop_state.retval = 17;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "async req: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "to-on refs: %u", srv.refs);
	check_result(-EAGAIN, "async req");
	zassert_equal(ntrans, 1U,
		      "async req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");

	notify(&start_state);
	zassert_equal(srv.refs, 1U,
		      "on refs: %u", srv.refs);
	check_result(start_state.retval, "async req");
	zassert_equal(ntrans, 2U,
		      "async req trans");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");

	rc = onoff_release(&srv);
	zassert_true(rc >= 0,
		     "rel: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "on refs: %u", srv.refs);
	zassert_equal(ntrans, 3U,
		      "async rel trans");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");

	notify(&stop_state);
	zassert_equal(srv.refs, 0U,
		      "rel refs: %u", srv.refs);
	zassert_equal(ntrans, 4U,
		      "rel trans");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");
}

ZTEST(onoff_api, test_reset)
{
	struct onoff_client cli2 = {};
	int rc;

	setup_error();

	reset_cli();
	rc = onoff_reset(NULL, NULL);
	zassert_equal(rc, -EINVAL,
		      "rst 0 0");
	rc = onoff_reset(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "rst srv 0");
	rc = onoff_reset(NULL, &cli);
	zassert_equal(rc, -EINVAL,
		      "rst 0 cli");
	rc = onoff_reset(&srv, &cli2);
	zassert_equal(rc, -EINVAL,
		      "rst srv cli-cfg");

	transitions.reset = NULL;
	rc = onoff_reset(&srv, &cli);
	zassert_equal(rc, -ENOTSUP,
		      "rst srv cli-cfg");

	transitions.reset = reset;
	rc = onoff_reset(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_ERROR,
		      "rst srv cli");

	reset_cli();
	rc = onoff_reset(&srv, &cli);
	zassert_equal(rc, -EALREADY,
		      "re-rst srv cli");
}

ZTEST(onoff_api, test_basic_reset)
{
	int rc;

	/* Verify that reset works. */

	setup_error();

	zassert_equal(ntrans, 2U,
		      "err trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(1, ONOFF_STATE_ERROR, start_state.retval,
		   "trans on");

	reset_cli();
	reset_state.retval = 12;

	rc = onoff_reset(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_ERROR,
		      "rst");
	check_result(reset_state.retval,
		     "rst");
	zassert_equal(ntrans, 4U,
		      "err trans");
	check_trans(2, ONOFF_STATE_RESETTING, 0,
		   "trans resetting");
	check_trans(3, ONOFF_STATE_OFF, reset_state.retval,
		   "trans off");
}

ZTEST(onoff_api, test_multi_start)
{
	int rc;
	struct onoff_client cli2 = {};

	/* Verify multiple requests are satisfied when start
	 * transition completes.
	 */

	setup_test();

	start_state.async = true;
	start_state.retval = 16;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "req refs: %u", srv.refs);
	check_result(-EAGAIN, "req");
	zassert_equal(ntrans, 1U,
		      "req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");

	sys_notify_init_spinwait(&cli2.notify);

	rc = onoff_request(&srv, &cli2);
	zassert_equal(rc, ONOFF_STATE_TO_ON,
		      "req2: %d", rc);
	zassert_equal(cli_result(&cli2), -EAGAIN,
		      "req2 result");

	notify(&start_state);

	zassert_equal(ntrans, 2U,
		      "async req trans");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");
	check_result(start_state.retval, "req");
	zassert_equal(cli_result(&cli2), start_state.retval,
		      "req2");
}

ZTEST(onoff_api, test_indep_req)
{
	int rc;
	struct onoff_client cli0 = {};

	/* Verify that requests and releases while on behave as
	 * expected.
	 */

	setup_test();
	sys_notify_init_spinwait(&cli0.notify);
	start_state.retval = 62;

	rc = onoff_request(&srv, &cli0);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req0: %d", rc);
	zassert_equal(srv.refs, 1U,
		      "req0 refs: %u", srv.refs);
	zassert_equal(cli_result(&cli0), start_state.retval,
		      "req0 result");
	zassert_equal(ntrans, 2U,
		      "req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");

	++start_state.retval;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_ON,
		      "req: %d", rc);
	check_result(0,
		     "req");

	zassert_equal(ntrans, 2U,
		      "async req trans");
	zassert_equal(srv.refs, 2U,
		      "srv refs: %u", srv.refs);

	rc = onoff_release(&srv); /* pair with cli0 */
	zassert_equal(rc, ONOFF_STATE_ON,
		      "rel: %d", rc);
	zassert_equal(srv.refs, 1U,
		      "srv refs");
	zassert_equal(ntrans, 2U,
		      "async req trans");

	rc = onoff_release(&srv); /* pair with cli */
	zassert_equal(rc, ONOFF_STATE_ON,
		      "rel: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "srv refs");
	zassert_equal(ntrans, 4U,
		      "async req trans");
}

ZTEST(onoff_api, test_delayed_req)
{
	int rc;

	setup_test();
	start_state.retval = 16;

	/* Verify that a request received while turning off is
	 * processed on completion of the transition to off.
	 */

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req: %d", rc);
	check_result(start_state.retval, "req");
	zassert_equal(ntrans, 2U,
		      "req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");

	start_state.retval += 1;
	stop_state.async = true;
	stop_state.retval = 14;

	rc = onoff_release(&srv);
	zassert_true(rc >= 0,
		     "rel: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "on refs: %u", srv.refs);
	zassert_equal(ntrans, 3U,
		      "async rel trans");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");

	reset_cli();

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_TO_OFF,
		      "del req: %d", rc);
	zassert_equal(ntrans, 3U,
		      "async rel trans");
	check_result(-EAGAIN, "del req");

	notify(&stop_state);

	check_result(start_state.retval, "del req");
	zassert_equal(ntrans, 6U,
		      "req trans");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");
	check_trans(4, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(5, ONOFF_STATE_ON, start_state.retval,
		   "trans on");
}


ZTEST(onoff_api, test_recheck_start)
{
	int rc;

	/* Verify fast-path recheck when entering ON with no clients.
	 *
	 * This removes the monitor which bypasses the unlock region
	 * in process_events() when there is no client and no
	 * transition.
	 */

	setup_test();
	rc = onoff_monitor_unregister(&srv, &mon);
	zassert_equal(rc, 0,
		      "mon unreg");

	start_state.async = true;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req");
	rc = onoff_cancel(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_TO_ON,
		      "cancel");

	notify(&start_state);
	zassert_equal(srv.flags, ONOFF_STATE_OFF,
		      "completed");
}

ZTEST(onoff_api, test_recheck_stop)
{
	int rc;

	/* Verify fast-path recheck when entering OFF with clients.
	 *
	 * This removes the monitor which bypasses the unlock region
	 * in process_events() when there is no client and no
	 * transition.
	 */

	setup_test();
	rc = onoff_monitor_unregister(&srv, &mon);
	zassert_equal(rc, 0,
		      "mon unreg");

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req");
	check_result(start_state.retval,
		     "req");

	stop_state.async = true;
	rc = onoff_release(&srv);
	zassert_equal(rc, ONOFF_STATE_ON,
		      "rel");

	reset_cli();
	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_TO_OFF,
		      "delayed req");
	check_result(-EAGAIN,
		     "delayed req");

	notify(&stop_state);
	zassert_equal(srv.flags, ONOFF_STATE_ON,
		      "completed");
}

static void rel_in_req_cb(struct onoff_manager *srv,
			  struct onoff_client *cli,
			  uint32_t state,
			  int res)
{
	int rc = onoff_release(srv);

	zassert_equal(rc, ONOFF_STATE_ON,
		      "rel-in-req");
}

ZTEST(onoff_api, test_rel_in_req_cb)
{
	int rc;

	/* Verify that a release invoked during the request completion
	 * callback is processed to final state.
	 */

	setup_test();
	callback_fn = rel_in_req_cb;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req");

	zassert_equal(callback_fn, NULL,
		      "invoke");

	zassert_equal(ntrans, 4U,
		      "req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");
}

ZTEST(onoff_api, test_multi_reset)
{
	int rc;
	struct onoff_client cli2 = {};

	/* Verify multiple reset requests are satisfied when reset
	 * transition completes.
	 */
	setup_test();
	start_state.retval = -23;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req err");
	check_result(start_state.retval,
		     "req err");
	zassert_true(onoff_has_error(&srv),
		     "has_error");
	zassert_equal(ntrans, 2U,
		      "err trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(1, ONOFF_STATE_ERROR, start_state.retval,
		   "trans on");

	reset_state.async = true;
	reset_state.retval = 21;

	sys_notify_init_spinwait(&cli2.notify);
	rc = onoff_reset(&srv, &cli2);
	zassert_equal(rc, ONOFF_STATE_ERROR,
		      "rst2");
	zassert_equal(cli_result(&cli2), -EAGAIN,
		      "rst2 result");
	zassert_equal(ntrans, 3U,
		      "rst trans");
	check_trans(2, ONOFF_STATE_RESETTING, 0,
		   "trans resetting");

	reset_cli();
	rc = onoff_reset(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_RESETTING,
		      "rst");
	zassert_equal(ntrans, 3U,
		      "rst trans");

	notify(&reset_state);

	zassert_equal(cli_result(&cli2), reset_state.retval,
		      "rst2 result");
	check_result(reset_state.retval,
		     "rst");
	zassert_equal(ntrans, 4U,
		      "rst trans");
	check_trans(3, ONOFF_STATE_OFF, reset_state.retval,
		   "trans off");
}

ZTEST(onoff_api, test_error)
{
	struct onoff_client cli2 = {};
	int rc;

	/* Verify rejected operations when error present. */

	setup_error();

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -EIO,
		      "req in err");

	rc = onoff_release(&srv);
	zassert_equal(rc, -EIO,
		      "rel in err");

	reset_state.async = true;
	sys_notify_init_spinwait(&cli2.notify);

	rc = onoff_reset(&srv, &cli2);
	zassert_equal(rc, ONOFF_STATE_ERROR,
		      "rst");

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, -ENOTSUP,
		      "req in err");

	rc = onoff_release(&srv);
	zassert_equal(rc, -ENOTSUP,
		      "rel in err");
}

ZTEST(onoff_api, test_cancel_req)
{
	int rc;

	setup_test();
	start_state.async = true;
	start_state.retval = 14;

	rc = onoff_cancel(NULL, NULL);
	zassert_equal(rc, -EINVAL,
		      "can 0 0");
	rc = onoff_cancel(&srv, NULL);
	zassert_equal(rc, -EINVAL,
		      "can srv 0");
	rc = onoff_cancel(NULL, &cli);
	zassert_equal(rc, -EINVAL,
		      "can 0 cli");

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "async req: %d", rc);
	check_result(-EAGAIN, "async req");
	zassert_equal(ntrans, 1U,
		      "req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");

	rc = onoff_cancel(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_TO_ON,
		      "cancel req: %d", rc);

	rc = onoff_cancel(&srv, &cli);
	zassert_equal(rc, -EALREADY,
		      "re-cancel req: %d", rc);

	zassert_equal(ntrans, 1U,
		      "req trans");
	notify(&start_state);

	zassert_equal(ntrans, 4U,
		      "req trans");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");
}

ZTEST(onoff_api, test_cancel_delayed_req)
{
	int rc;

	setup_test();

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req: %d", rc);
	check_result(start_state.retval, "req");
	zassert_equal(ntrans, 2U,
		      "req trans");
	check_trans(0, ONOFF_STATE_TO_ON, 0,
		   "trans to-on");
	check_trans(1, ONOFF_STATE_ON, start_state.retval,
		   "trans on");

	stop_state.async = true;
	stop_state.retval = 14;

	rc = onoff_release(&srv);
	zassert_true(rc >= 0,
		     "rel: %d", rc);
	zassert_equal(srv.refs, 0U,
		      "on refs: %u", srv.refs);
	zassert_equal(ntrans, 3U,
		      "async rel trans");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");

	reset_cli();

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_TO_OFF,
		      "del req: %d", rc);
	zassert_equal(ntrans, 3U,
		      "async rel trans");
	check_result(-EAGAIN, "del req");

	rc = onoff_cancel(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_TO_OFF,
		      "can del req: %d", rc);

	notify(&stop_state);

	zassert_equal(ntrans, 4U,
		      "req trans");
	check_trans(2, ONOFF_STATE_TO_OFF, 0,
		   "trans to-off");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");
}

ZTEST(onoff_api, test_cancel_or_release)
{
	int rc;

	/* First, verify that the cancel-or-release idiom works when
	 * invoked in state TO-ON.
	 */

	setup_test();
	start_state.async = true;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req");
	rc = onoff_cancel_or_release(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_TO_ON,
		      "c|r to-on");
	notify(&start_state);

	zassert_equal(ntrans, 4U,
		      "req trans");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");

	/* Now verify that the cancel-or-release idiom works when
	 * invoked in state ON.
	 */

	setup_test();
	start_state.async = false;

	rc = onoff_request(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_OFF,
		      "req");
	zassert_equal(ntrans, 2U,
		      "req trans");

	rc = onoff_cancel_or_release(&srv, &cli);
	zassert_equal(rc, ONOFF_STATE_ON,
		      "c|r to-on");
	zassert_equal(ntrans, 4U,
		      "req trans");
	check_trans(3, ONOFF_STATE_OFF, stop_state.retval,
		   "trans off");
}

ZTEST(onoff_api, test_sync_basic)
{
	static struct onoff_sync_service srv = {};
	k_spinlock_key_t key;
	int res = 5;
	int rc;

	reset_cli();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, 0,
		      "init req");

	rc = onoff_sync_finalize(&srv, key, &cli, res, true);
	zassert_equal(rc, 1,
		      "req count");
	zassert_equal(callback_srv, NULL,
		      "sync cb srv");
	zassert_equal(callback_cli, &cli,
		      "sync cb cli");
	check_callback(ONOFF_STATE_ON, res, "sync req");

	reset_cli();
	reset_callback();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, 1,
		      "init rel");

	++res;
	rc = onoff_sync_finalize(&srv, key, &cli, res, true);
	zassert_equal(rc, 2,
		      "req2 count");
	check_callback(ONOFF_STATE_ON, res, "sync req2");

	reset_cli();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, 2,
		      "init rel");

	rc = onoff_sync_finalize(&srv, key, NULL, res, false);
	zassert_equal(rc, 1,
		      "rel count");

	reset_cli();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, 1,
		      "init rel2");

	rc = onoff_sync_finalize(&srv, key, NULL, res, false);
	zassert_equal(rc, 0,
		      "rel2 count");

	/* Extra release is caught and diagnosed.  May not happen with
	 * onoff manager, but we can/should do it for sync.
	 */
	reset_cli();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, 0,
		      "init rel2");

	rc = onoff_sync_finalize(&srv, key, NULL, res, false);
	zassert_equal(rc, -1,
		      "rel-1 count");

	/* Error state is visible to next lock. */
	reset_cli();
	reset_callback();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, -1,
		      "init req");
}

ZTEST(onoff_api, test_sync_error)
{
	static struct onoff_sync_service srv = {};
	k_spinlock_key_t key;
	int res = -EPERM;
	int rc;

	reset_cli();
	reset_callback();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, 0,
		      "init req");

	rc = onoff_sync_finalize(&srv, key, &cli, res, true);

	zassert_equal(rc, res,
		      "err final");
	zassert_equal(srv.count, res,
		      "srv err count");
	zassert_equal(callback_srv, NULL,
		      "sync cb srv");
	zassert_equal(callback_cli, &cli,
		      "sync cb cli");
	check_callback(ONOFF_STATE_ERROR, res, "err final");

	/* Error is visible to next operation (the value is the
	 * negative count)
	 */

	reset_cli();
	reset_callback();

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, -1,
		      "init req");

	/* Error is cleared by non-negative finalize result */
	res = 3;
	rc = onoff_sync_finalize(&srv, key, &cli, res, true);

	zassert_equal(rc, 1,
		      "req count %d", rc);
	check_callback(ONOFF_STATE_ON, res, "sync req");

	rc = onoff_sync_lock(&srv, &key);
	zassert_equal(rc, 1,
		      "init rel");
}


void *test_init(void)
{
	k_sem_init(&isr_sync, 0, 1);
	k_timer_init(&isr_timer, isr_notify, NULL);

	(void)isr_reset;
	(void)isr_release;
	(void)isr_request;
	return NULL;
}
ZTEST_SUITE(onoff_api, NULL, test_init, NULL, NULL, NULL);
