/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/seq_mgr.h>

#define PAUSE_FLAG BIT(0)

#define ZASSERT_EQUAL(x, y, str, ...) \
		zassert_equal(x, y, "callee line %d:" str, line, ##__VA_ARGS__)
#define ZASSERT_TRUE(x, str, ...) \
	zassert_true(x, "callee line %d:" str, line, ##__VA_ARGS__)

struct service_msg {
	uint32_t delay;
	int err;
	int callback_err;
	int executed_cnt;
	int skip;
	int repeat;
};

struct custom_process_msg {
	sys_seq_action_process func;
	struct service_msg msg;
};


static int seq_setup(struct sys_seq_mgr *mgr,
		const struct sys_seq *seq);

static int seq_teardown(struct sys_seq_mgr *mgr,
		const struct sys_seq *seq, int actions, int res);

static int action_process(struct sys_seq_mgr *mgr,
			void *action);

struct sys_seq_functions functions = {
	.setup = seq_setup,
	.teardown = seq_teardown,
	.action_process = action_process
};

struct sys_seq_functions functions_no_process = {
	.setup = seq_setup,
	.teardown = seq_teardown
};

struct sys_seq_functions functions_no_setup = {
	.teardown = seq_teardown,
	.action_process = action_process
};

struct sys_seq_functions functions_no_teardown = {
	.setup = seq_setup,
	.action_process = action_process
};

struct setup_teardown {
	int cnt;
	int callback_err;
	int err;
};

struct mock_service {
	struct sys_seq_mgr mgr;
	struct k_timer timer;
	struct setup_teardown setup;
	int teardown_res;
	int teardown_actions;
	struct setup_teardown teardown;
	int callback_err;
	bool skip_exec_check;
	int tmp_offset;
};

struct mock_service service = {
	.mgr = {
		.vtable = &functions
	}
};

static int seq_setup_teardown(struct sys_seq_mgr *mgr,
				struct setup_teardown *setup_teardown)
{
	setup_teardown->cnt++;
	if (setup_teardown->err < 0) {
		return setup_teardown->err;
	}

	sys_seq_finalize(mgr, setup_teardown->callback_err, 0);
	return 0;
}

static int seq_setup(struct sys_seq_mgr *mgr,
		const struct sys_seq *seq)
{
	struct mock_service *srv = CONTAINER_OF(mgr, struct mock_service, mgr);

	return seq_setup_teardown(mgr, &srv->setup);
}

static int seq_teardown(struct sys_seq_mgr *mgr,
		const struct sys_seq *seq, int actions, int res)
{
	struct mock_service *srv = CONTAINER_OF(mgr, struct mock_service, mgr);

	srv->teardown_actions = actions;
	srv->teardown_res = res;
	if (res < 0) {
		srv->teardown.callback_err = res;
	}

	return seq_setup_teardown(mgr, &srv->teardown);
}

static void timeout(struct k_timer *timer)
{
	struct mock_service *srv = k_timer_user_data_get(timer);

	sys_seq_finalize(&srv->mgr, srv->callback_err, srv->tmp_offset);
}

static int action_process(struct sys_seq_mgr *mgr,
			void *action)
{
	struct service_msg *msg = action;
	struct mock_service *srv = CONTAINER_OF(mgr, struct mock_service, mgr);
	int offset = 0;

	msg->executed_cnt++;
	if (msg->err < 0) {
		return msg->err;
	}

	if (msg->repeat) {
		offset = -1;
		msg->repeat--;
	} else if (msg->skip) {
		offset = msg->skip;
	}

	if (msg->delay == 0) {
		sys_seq_finalize(mgr, msg->callback_err, offset);
		return 0;
	}

	srv->callback_err = msg->callback_err;
	srv->tmp_offset = offset;

	k_timer_user_data_set(&srv->timer, srv);
	k_timer_start(&srv->timer, K_MSEC(msg->delay), K_NO_WAIT);
	return 0;
}

static void service_init(struct mock_service *srv,
			 const struct sys_seq_functions *vtable)
{
	srv->mgr.vtable = vtable;
	srv->setup.cnt = 0;
	srv->setup.err = 0;
	srv->setup.callback_err = 0;
	srv->teardown.cnt = 0;
	srv->teardown.err = 0;
	srv->teardown.callback_err = 0;
	k_timer_stop(&srv->timer);
	k_timer_init(&srv->timer, timeout, NULL);
}

static void msgs_init(union sys_seq_action *actions_list,
			struct service_msg *msgs,
			int len, uint32_t delay)
{
	for (int i = 0; i < len; i++) {
		msgs[i].err = 0;
		msgs[i].executed_cnt = 0;
		msgs[i].callback_err = 0;
		msgs[i].delay = delay;
		msgs[i].skip = 0;
		msgs[i].repeat = 0;

		actions_list[i].generic = &msgs[i];
	}
}
static void check_messages_executed(const union sys_seq_action *actions,
				    int executed, int not_executed, int line)
{
	struct service_msg **msgs = (struct service_msg **)actions;

	for (int i = 0; i < executed; i++) {
		ZASSERT_TRUE(msgs[i]->executed_cnt > 0, "");
	}
	for (int i = executed; i < (executed + not_executed); i++) {
		ZASSERT_TRUE(msgs[i]->executed_cnt == 0, "");
	}
}

static int msg_with_error(const union sys_seq_action *actions, int num_msgs)
{
	struct service_msg **msgs = (struct service_msg **)actions;

	for (int i = 0; i < num_msgs; i++) {
		if ((msgs[i]->callback_err < 0) || (msgs[i]->err < 0)) {
			return i;
		}
	}

	return -1;
}

static void execute_and_validate_sequence(struct mock_service *srv,
					struct sys_seq *seq, int line)
{
	struct sys_notify notify;
	int err;
	uint32_t stamp = k_uptime_get_32();
	struct service_msg **msgs = (struct service_msg **)seq->actions;

	sys_notify_init_spinwait(&notify);
	err = sys_seq_process(&srv->mgr, seq, &notify);

	/* Check case when setup returns error. */
	if (srv->mgr.vtable->setup && (srv->setup.err < 0)) {
		ZASSERT_EQUAL(srv->setup.cnt, 1,
				"Unexpected cnt: %d", srv->setup.cnt);
		ZASSERT_EQUAL(srv->setup.err, err, "Unexpected err: %d", err);
		/* check no messages executed. */
		check_messages_executed(seq->actions, 0,
					seq->num_actions, line);
		/* teardown not executed. */
		ZASSERT_EQUAL(srv->teardown.cnt, 0,
				"Unexpected cnt: %d", srv->teardown.cnt);
		return;
	} else if ((srv->mgr.vtable->setup == NULL) && (msgs[0]->err < 0)) {
		ZASSERT_EQUAL(msgs[0]->err, err, "Unexpected err: %d", err);
		/* check no messages executed. */
		check_messages_executed(seq->actions, 1, seq->num_actions - 1,
					line);
		/* teardown not executed. */
		ZASSERT_EQUAL(srv->teardown.cnt, 0,
				"Unexpected cnt: %d", srv->teardown.cnt);
		return;
	}

	ZASSERT_TRUE(err >= 0, "");

	while (sys_notify_fetch_result(&notify, &err) < 0) {

		/* watchdog */
		if (k_uptime_get_32() - stamp > 1000) {
			ZASSERT_TRUE(false, "Operation not completed on time");
		}
	}

	/* If setup returns error through callback other parts should not
	 * execute.
	 */
	if (srv->mgr.vtable->setup && (srv->setup.callback_err < 0)) {
		ZASSERT_EQUAL(srv->setup.cnt, 1,
				"Unexpected cnt: %d", srv->setup.cnt);
		ZASSERT_EQUAL(srv->setup.callback_err, err,
				"Unexpected err: %d", err);
		/* check no messages executed. */
		check_messages_executed(seq->actions, 0,
					seq->num_actions, line);
		/* teardown not executed. */
		ZASSERT_EQUAL(srv->teardown.cnt, 1,
				"Unexpected cnt: %d", srv->teardown.cnt);
		return;
	}


	/* Full sequence execution */
	if (srv->mgr.vtable->setup) {
		ZASSERT_EQUAL(srv->setup.cnt, 1, "");
	}

	int idx = msg_with_error(seq->actions, seq->num_actions);
	/* negative idx - no errors */
	if (idx >= 0) {
		int exp_err;

		check_messages_executed(seq->actions, idx + 1,
					seq->num_actions - (idx + 1), line);
		exp_err = (msgs[idx]->err < 0) ?
				msgs[idx]->err : msgs[idx]->callback_err;
		ZASSERT_EQUAL(exp_err, err, "Unexpected err: %d", err);

		if (srv->mgr.vtable->teardown) {
			ZASSERT_EQUAL(srv->teardown_res, exp_err,
					"Unexpected err: %d (exp: %d)",
					srv->teardown_res, exp_err);
			ZASSERT_EQUAL(srv->teardown.cnt, 1, "");
		}

		return;
	}

	if (srv->mgr.vtable->teardown) {
		ZASSERT_EQUAL(srv->teardown.cnt, 1, "");
		if (srv->teardown.err < 0) {
			ZASSERT_EQUAL(err, srv->teardown.err, "");
		} else if (srv->teardown.callback_err < 0) {
			ZASSERT_EQUAL(err, srv->teardown.callback_err, "");
		} else {
			ZASSERT_TRUE(err >= 0, "");
		}
	} else {
		ZASSERT_TRUE(err >= 0, "");
	}

	if (!srv->skip_exec_check) {
		check_messages_executed(seq->actions, seq->num_actions,
					0, line);
	}
}

#define EXECUTE_AND_VALIDATE_SEQUENCE(srv, seq) \
	execute_and_validate_sequence(srv, seq, __LINE__)

/* Setup a sequence consisting of one message which executes synchronously and
 * returns success. Setup and teardown function to also return no error.
 */
void test_single_action_sequence_execution_async(bool async)
{
	uint32_t delay = async ? 2 : 0;
	struct service_msg msgs[1];
	union sys_seq_action msglist[1];
	struct sys_seq seq = {
		.actions = msglist,
		.num_actions = ARRAY_SIZE(msgs)
	};

	/* Service with setup and teardown handlers */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service without setup callback */
	service_init(&service, &functions_no_setup);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service without teardown callback */
	service_init(&service, &functions_no_teardown);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service with setup returning error */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	service.setup.err = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service with setup returning asynchronous error */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	service.setup.callback_err = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service without setup returning error on first action*/
	service_init(&service, &functions_no_setup);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	msgs[0].err = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service returning error in first action's callback */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	msgs[0].callback_err = -EFAULT;
	service.teardown_res = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service returning error in teardown */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	service.teardown.err = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service returning error in teardown callback */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	service.teardown.callback_err = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);
}

void test_single_action_sequence_execution(void)
{
	test_single_action_sequence_execution_async(false);
	if (!IS_ENABLED(CONFIG_SOC_POSIX)) {
		test_single_action_sequence_execution_async(true);
	}
}

void test_multi_action_sequence_execution_async(bool async)
{
	uint32_t delay = async ? 2 : 0;
	struct service_msg msgs[3];
	union sys_seq_action msglist[ARRAY_SIZE(msgs)];
	struct sys_seq seq = {
		.actions = msglist,
		.num_actions = ARRAY_SIZE(msgs)
	};

	/* Service returning error in middle action's call */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	msgs[1].err = -EFAULT;
	service.teardown_res = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);

	/* Service returning error in middle action's callback */
	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), delay);
	msgs[1].callback_err = -EFAULT;
	service.teardown_res = -EFAULT;
	EXECUTE_AND_VALIDATE_SEQUENCE(&service, &seq);
}

void test_multi_action_sequence_execution(void)
{
	test_multi_action_sequence_execution_async(false);
	if (!IS_ENABLED(CONFIG_SOC_POSIX)) {
		test_multi_action_sequence_execution_async(true);
	}
}

void test_abort(void)
{
	struct service_msg msgs[3];
	union sys_seq_action msglist[ARRAY_SIZE(msgs)];
	struct sys_seq seq = {
		.actions = msglist,
		.num_actions = ARRAY_SIZE(msgs)
	};
	int err;
	struct sys_notify notify;
	uint32_t stamp = k_uptime_get_32();

	/* In posix asynchronous operations with k_timer cannot be achieved. */
	if (IS_ENABLED(CONFIG_SOC_POSIX)) {
		return;
	}

	service_init(&service, &functions);
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), 100);
	sys_notify_init_spinwait(&notify);

	err = sys_seq_abort(&service.mgr);
	zassert_equal(err, -EINVAL, "Unexpected err:%d", err);

	err = sys_seq_process(&service.mgr, &seq, &notify);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	err = sys_seq_abort(&service.mgr);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	while (sys_notify_fetch_result(&notify, &err) < 0) {
		/* watchdog */
		if (k_uptime_get_32() - stamp > 1000) {
			zassert_true(false, "Operation not completed on time");
		}
	}
	zassert_equal(err, -ECANCELED, "Unexpected err:%d", err);
}

void test_actions_jumping(void)
{
	struct service_msg msgs[3];
	union sys_seq_action msglist[ARRAY_SIZE(msgs)];
	struct sys_seq seq = {
		.actions = msglist,
		.num_actions = ARRAY_SIZE(msgs)
	};
	int err;
	struct sys_notify notify;
	uint32_t stamp = k_uptime_get_32();

	service_init(&service, &functions);
	service.skip_exec_check = true;
	msgs_init(msglist, msgs, ARRAY_SIZE(msgs), 0);
	msgs[0].repeat = 3; /* msg0 should be repeated 3 times. */
	msgs[1].skip = 1;

	sys_notify_init_spinwait(&notify);

	err = sys_seq_process(&service.mgr, &seq, &notify);
	zassert_equal(err, 0, "Unexpected err:%d", err);

	while (sys_notify_fetch_result(&notify, &err) < 0) {
		/* watchdog */
		if (k_uptime_get_32() - stamp > 1000) {
			zassert_true(false, "Operation not completed on time");
		}
	}
	zassert_equal(err, 0, "Unexpected err:%d", err);
	/* First message is executed once and then 3 times repeated. */
	zassert_equal(msgs[0].executed_cnt, 3 + 1, "Unexpected count value: %d",
			msgs[0].executed_cnt);
	zassert_equal(msgs[1].executed_cnt, 1, "Unexpected count value: %d",
			msgs[1].executed_cnt);
	zassert_equal(msgs[2].executed_cnt, 0, "Unexpected count value: %d",
			msgs[2].executed_cnt);
}

void test_custom_processor(void)
{
	int err;
	struct sys_notify notify;
	struct custom_process_msg msg;
	union sys_seq_action msgptr;
	struct sys_seq seq = {
		.actions = &msgptr,
		.num_actions = 1
	};

	/* Service with setup and teardown handlers */
	service_init(&service, &functions_no_process);
	memset(&msg, 0, sizeof(msg));
	msgptr.custom = (struct sys_seq_func_action *)&msg;
	msg.func = action_process;

	sys_notify_init_spinwait(&notify);
	err = sys_seq_process(&service.mgr, &seq, &notify);
	zassert_true(err >= 0, NULL);
	zassert_equal(msg.msg.executed_cnt, 1, NULL);
}

struct test_data {
	int x;
	int y;
};

static int sum;

static int process(struct sys_seq_mgr *mgr, void *action)
{
	struct test_data *data = (struct test_data *)action;

	sum += (data->x + data->y);

	sys_seq_finalize(mgr, 0, 0);

	return 0;
}

void test_SYS_SEQ_DEFINE(void)
{

	SYS_SEQ_DEFINE(static const, const_seq,
	    SYS_SEQ_ACTION(static const, struct test_data, ({.x = 1, .y = 2})),
	    SYS_SEQ_ACTION(static const, struct test_data, ({.x = 3, .y = 4}))
	);


	SYS_SEQ_DEFINE(, rw_seq,
		SYS_SEQ_ACTION(, struct test_data, ({.x = 1, .y = 1})),
		SYS_SEQ_ACTION(, struct test_data, ({.x = 1, .y = 1}))
	);

	int err;
	int ret;
	static const struct sys_seq_functions functions = {
		.action_process = process
	};
	struct sys_seq_mgr mgr = {0};
	struct sys_notify notify;

	mgr.vtable = &functions;

	sum = 0;

	sys_notify_init_spinwait(&notify);
	err = sys_seq_process(&mgr, &const_seq, &notify);
	zassert_true(err >= 0, NULL);

	err = sys_notify_fetch_result(&notify, &ret);
	zassert_true(err >= 0, NULL);
	zassert_true(ret >= 0, NULL);
	zassert_equal(sum, 10, "Unexpected sum, sequence not performed?");

	sys_notify_init_spinwait(&notify);
	err = sys_seq_process(&mgr, &rw_seq, &notify);
	zassert_true(err >= 0, NULL);

	err = sys_notify_fetch_result(&notify, &ret);
	zassert_true(err >= 0, NULL);
	zassert_true(ret >= 0, NULL);
	zassert_equal(sum, 14, "Unexpected sum, sequence not performed?");
}

void test_custom_SYS_SEQ_DEFINE(void)
{
	SYS_SEQ_DEFINE(static const, const_seq,
		SYS_SEQ_CUSTOM_ACTION(process, static const, struct test_data,
				     ({.x = 1, .y = 2})),
		SYS_SEQ_CUSTOM_ACTION(process, static const, struct test_data,
				    ({.x = 3, .y = 4}))
	);


	SYS_SEQ_DEFINE(, rw_seq,
		SYS_SEQ_CUSTOM_ACTION(process, /* empty */, struct test_data,
					({.x = 1, .y = 1})),
		SYS_SEQ_CUSTOM_ACTION(process, /* empty */, struct test_data,
					({.x = 1, .y = 1}))
	);

	int err;
	int ret;
	/* no process function */
	static const struct sys_seq_functions functions = {};
	struct sys_seq_mgr mgr = {0};
	struct sys_notify notify;

	mgr.vtable = &functions;

	sum = 0;

	sys_notify_init_spinwait(&notify);
	err = sys_seq_process(&mgr, &const_seq, &notify);
	zassert_true(err >= 0, NULL);

	err = sys_notify_fetch_result(&notify, &ret);
	zassert_true(err >= 0, NULL);
	zassert_true(ret >= 0, NULL);
	zassert_equal(sum, 10, "Unexpected sum, sequence not performed?");

	sys_notify_init_spinwait(&notify);
	err = sys_seq_process(&mgr, &rw_seq, &notify);
	zassert_true(err >= 0, NULL);

	err = sys_notify_fetch_result(&notify, &ret);
	zassert_true(err >= 0, NULL);
	zassert_true(ret >= 0, NULL);
	zassert_equal(sum, 14, "Unexpected sum, sequence not performed?");
}

void test_generic_delay_ms_action(void)
{
	#undef DELAY
	#define DELAY 100
	int err;
	int ret;
	struct k_timer timer;
	uint32_t stamp;
	static const struct sys_seq_functions functions = {
		/* no process function */
	};
	struct sys_seq_mgr mgr = {0};
	struct sys_notify notify;

	SYS_SEQ_DEFINE(static const, seq,
		SYS_SEQ_ACTION_MS_DELAY(DELAY)
	);

	err = sys_seq_mgr_init(&mgr, &functions, &timer);
	zassert_true(err >= 0, NULL);

	sys_notify_init_spinwait(&notify);
	stamp = k_uptime_get();
	err = sys_seq_process(&mgr, &seq, &notify);
	zassert_true(err >= 0, NULL);

	while (sys_notify_fetch_result(&notify, &ret) < 0) {
		/* needed by native_posix */
		k_busy_wait(1000);
	}

	stamp = k_uptime_get() - stamp;
	zassert_true(err >= 0, NULL);
	zassert_true((stamp >= DELAY) && (stamp < (1.2 * DELAY)), NULL);
}

void test_generic_delay_us_action(void)
{
	#undef DELAY
	#define DELAY 100
	int err;
	int ret;
	struct k_timer timer;
	uint32_t stamp;
	static const struct sys_seq_functions functions = {
		/* no process function */
	};
	struct sys_seq_mgr mgr = {0};
	struct sys_notify notify;

	SYS_SEQ_DEFINE(static const, seq,
		SYS_SEQ_ACTION_US_DELAY(1000*DELAY)
	);

	err = sys_seq_mgr_init(&mgr, &functions, &timer);
	zassert_true(err >= 0, NULL);

	sys_notify_init_spinwait(&notify);
	stamp = k_uptime_get();
	err = sys_seq_process(&mgr, &seq, &notify);
	zassert_true(err >= 0, NULL);

	while (sys_notify_fetch_result(&notify, &ret) < 0) {
		/* needed by native_posix */
		k_busy_wait(1000);
	}

	stamp = k_uptime_get() - stamp;
	zassert_true(err >= 0, NULL);
	zassert_true((stamp >= DELAY) && (stamp < (1.2 * DELAY)), NULL);
}

static int pause_handler(struct sys_seq_mgr *mgr, void *action)
{
	uint32_t *delay = action;

	k_sleep(K_MSEC(*delay));
	sys_seq_finalize(mgr, 0, 0);

	return 0;
}

void test_generic_pause_action(void)
{
	#undef DELAY
	#define DELAY 100
	int err;
	int ret;
	struct k_timer timer;
	uint32_t stamp;
	static const struct sys_seq_functions functions = {
		/* no process function */
	};
	struct sys_seq_mgr mgr = {0};
	struct sys_notify notify;

	SYS_SEQ_DEFINE(static const, seq,
		SYS_SEQ_ACTION_PAUSE(pause_handler, DELAY)
	);

	err = sys_seq_mgr_init(&mgr, &functions, &timer);
	zassert_true(err >= 0, NULL);

	sys_notify_init_spinwait(&notify);
	stamp = k_uptime_get();
	err = sys_seq_process(&mgr, &seq, &notify);
	zassert_true(err >= 0, NULL);

	while (sys_notify_fetch_result(&notify, &ret) < 0) {
		/* needed by native_posix */
		k_busy_wait(1000);
	}

	stamp = k_uptime_get() - stamp;
	zassert_true(err >= 0, NULL);
	zassert_true((stamp >= DELAY) && (stamp < (1.2 * DELAY)), NULL);
}

void test_main(void)
{
	ztest_test_suite(sys_seq_mgr_api,
		ztest_unit_test(test_single_action_sequence_execution),
		ztest_unit_test(test_multi_action_sequence_execution),
		ztest_unit_test(test_abort),
		ztest_unit_test(test_actions_jumping),
		ztest_unit_test(test_custom_processor),
		ztest_unit_test(test_SYS_SEQ_DEFINE),
		ztest_unit_test(test_custom_SYS_SEQ_DEFINE),
		ztest_unit_test(test_generic_delay_ms_action),
		ztest_unit_test(test_generic_delay_us_action),
		ztest_unit_test(test_generic_pause_action)

	);
	ztest_run_test_suite(sys_seq_mgr_api);
}
