/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <ztest.h>
#include <logging/log_ctrl.h>

struct mock_log_frontend {
	bool do_check;
	bool panic;
	struct mock_log_backend_msg exp_msgs[64];
	int msg_rec_idx;
	int msg_proc_idx;
};

static struct mock_log_backend mock;
static struct log_backend_control_block cb = {
	.ctx = &mock
};

static const struct log_backend backend = {
	.cb = &cb
};

void mock_log_frontend_dummy_record(int cnt)
{
	mock_log_backend_dummy_record(&backend, cnt);
}

void mock_log_frontend_check_enable(void)
{
	mock.do_check = true;
}

void mock_log_frontend_check_disable(void)
{
	mock.do_check = false;
}

void mock_log_frontend_generic_record(uint16_t source_id,
				      uint16_t domain_id,
				      uint8_t level,
				      const char *str,
				      uint8_t *data,
				      uint32_t data_len)
{
	if (!IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		return;
	}

	mock_log_backend_generic_record(&backend, source_id, domain_id, level,
					(log_timestamp_t)UINT32_MAX, str, data, data_len);
}

void mock_log_frontend_validate(bool panic)
{
	if (!IS_ENABLED(CONFIG_LOG_FRONTEND)) {
		return;
	}

	mock_log_backend_validate(&backend, panic);
}

void mock_log_frontend_reset(void)
{
	mock_log_backend_reset(&backend);
}

struct test_str {
	char *str;
	int cnt;
};

static int out(int c, void *ctx)
{
	struct test_str *s = ctx;

	s->str[s->cnt++] = (char)c;

	return c;
}

static void log_frontend_n(struct log_msg_ids src_level, const char *fmt, ...)
{
	struct mock_log_backend_msg *exp_msg = &mock.exp_msgs[mock.msg_proc_idx];
	char str[128];
	va_list ap;

	if (mock.do_check == false) {
		return;
	}

	mock.msg_proc_idx++;

	if (!exp_msg->check) {
		return;
	}

	zassert_equal(src_level.level, exp_msg->level, NULL);
	zassert_equal(src_level.source_id, exp_msg->source_id, NULL);
	zassert_equal(src_level.domain_id, exp_msg->domain_id, NULL);

	va_start(ap, fmt);

	vsnprintk(str, sizeof(str), fmt, ap);

	va_end(ap);

	zassert_equal(0, strcmp(str, exp_msg->str), NULL);
}

void log_frontend_hexdump(const char *str,
			  const uint8_t *data,
			  uint32_t length,
			  struct log_msg_ids src_level)
{
	struct mock_log_backend_msg *exp_msg = &mock.exp_msgs[mock.msg_proc_idx];

	if (mock.do_check == false) {
		return;
	}

	zassert_equal(exp_msg->data_len, length, NULL);
	if (exp_msg->data_len <= sizeof(exp_msg->data)) {
		zassert_equal(memcmp(data, exp_msg->data, length), 0, NULL);
	}

	log_frontend_n(src_level, str);
}

void log_frontend_0(const char *str, struct log_msg_ids src_level)
{
	if (mock.do_check == false) {
		return;
	}

	log_frontend_n(src_level, str);
}

void log_frontend_1(const char *str, log_arg_t arg0, struct log_msg_ids src_level)
{
	log_frontend_n(src_level, str, arg0);
}

void log_frontend_2(const char *str, log_arg_t arg0, log_arg_t arg1, struct log_msg_ids src_level)
{
	if (mock.do_check == false) {
		return;
	}

	log_frontend_n(src_level, str, arg0, arg1);
}

void log_frontend_msg(const void *source,
		      const struct log_msg2_desc desc,
		      uint8_t *package, const void *data)
{
	struct mock_log_backend_msg *exp_msg = &mock.exp_msgs[mock.msg_proc_idx];

	if (mock.do_check == false) {
		return;
	}

	mock.msg_proc_idx++;

	if (!exp_msg->check) {
		return;
	}

	zassert_equal(desc.level, exp_msg->level, NULL);
	zassert_equal(desc.domain, exp_msg->domain_id, NULL);

	uint32_t source_id = IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
		log_dynamic_source_id((struct log_source_dynamic_data *)source) :
		log_const_source_id((const struct log_source_const_data *)source);

	zassert_equal(source_id, exp_msg->source_id, "got: %d, exp: %d",
			source_id, exp_msg->source_id);

	zassert_equal(exp_msg->data_len, desc.data_len, NULL);
	if (exp_msg->data_len <= sizeof(exp_msg->data)) {
		zassert_equal(memcmp(data, exp_msg->data, desc.data_len), 0, NULL);
	}

	char str[128];
	struct test_str s = { .str = str };
	size_t len = cbpprintf(out, &s, package);

	if (len > 0) {
		str[len] = '\0';
	}

	zassert_equal(strcmp(str, exp_msg->str), 0, "Got \"%s\", Expected:\"%s\"",
			str, exp_msg->str);
}

void log_frontend_panic(void)
{
	mock.panic = true;
}

void log_frontend_init(void)
{

}
