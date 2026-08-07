/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/tc_util.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

struct backend_context {
	uint32_t cnt;
	const char *exp_str[10];
	uint32_t delay;
	bool active;
	struct k_timer timer;
};

static int cbprintf_callback(int c, void *ctx)
{
	char **p = ctx;

	**p = c;
	(*p)++;

	return c;
}

static void backend_process(const struct log_backend *const backend,
			    union log_msg_generic *msg)
{
	char str[100];
	char *pstr = str;
	struct backend_context *context = (struct backend_context *)backend->cb->ctx;
	size_t len;
	uint8_t *p = log_msg_get_package(&msg->log, &len);

	(void)len;
	int slen = cbpprintf(cbprintf_callback, &pstr, p);

	str[slen] = '\0';

	zassert_equal(strcmp(str, context->exp_str[context->cnt]), 0,
			"Unexpected string %s (exp:%s)", str, context->exp_str[context->cnt]);

	context->cnt++;
}

static void panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static void expire_cb(struct k_timer *timer)
{
	void *ctx = k_timer_user_data_get(timer);
	struct backend_context *context = (struct backend_context *)ctx;

	context->active = true;
}

static void backend_init(const struct log_backend *const backend)
{
	struct backend_context *context = (struct backend_context *)backend->cb->ctx;

	k_timer_init(&context->timer, expire_cb, NULL);
	k_timer_user_data_set(&context->timer, (void *)context);
	k_timer_start(&context->timer, K_MSEC(context->delay), K_NO_WAIT);
}

static int backend_is_ready(const struct log_backend *const backend)
{
	struct backend_context *context = (struct backend_context *)backend->cb->ctx;

	return context->active ? 0 : -EBUSY;
}

static const struct log_backend_api backend_api = {
	.process = backend_process,
	.init = backend_init,
	.is_ready = backend_is_ready,
	.panic = panic
};

static struct backend_context context1 = {
	.delay = 500
};

static struct backend_context context2 = {
	.delay = 1000,
};

LOG_BACKEND_DEFINE(backend1, backend_api, true, &context1);
LOG_BACKEND_DEFINE(backend2, backend_api, true, &context2);

/* Test is using two backends which are configured to be autostarted but have
 * prolonged initialization. Backend1 starts earlier.
 *
 * Logging does not process logs until at least one backend is ready so once
 * backend1 is ready first log message is processed. Since backend2 is not yet
 * ready it will not receive this message. Second message is created when
 * both backends are initialized so both receives the message.
 */
ZTEST(log_backend_init, test_log_backends_initialization)
{
	int cnt;

	STRUCT_SECTION_COUNT(log_backend, &cnt);
	if (cnt != 2) {
		/* Other backends should not be enabled. */
		ztest_test_skip();
	}

	context1.cnt = 0;
	context2.cnt = 0;

	context1.exp_str[0] = "test1";
	context1.exp_str[1] = "test2";

	context2.exp_str[0] = "test2";

	LOG_INF("test1");

	/* Backends are not yet active. */
	zassert_false(context1.active);
	zassert_false(context2.active);

	k_msleep(context2.delay + 100);

	zassert_true(context1.active);
	zassert_true(context2.active);

	LOG_INF("test2");

	k_msleep(1100);

	/* Backend1 gets both messages but backend2 gets only second message
	 * because when first was processed it was not yet active.
	 */
	zassert_equal(context1.cnt, 2, "Unexpected value:%d (exp: %d)", context1.cnt, 2);
	zassert_equal(context2.cnt, 1);
}

ZTEST_SUITE(log_backend_init, NULL, NULL, NULL, NULL, NULL);
