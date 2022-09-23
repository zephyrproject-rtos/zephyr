/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <zephyr/ztest.h>
#include <zephyr/logging/log_ctrl.h>

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

void log_frontend_msg(const void *source,
		      const struct log_msg_desc desc,
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

	zassert_equal(desc.level, exp_msg->level);
	zassert_equal(desc.domain, exp_msg->domain_id);

	uint32_t source_id;

	if (desc.level == LOG_LEVEL_NONE) {
		source_id = (uintptr_t)source;
	} else {
		source_id = IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
			log_dynamic_source_id((struct log_source_dynamic_data *)source) :
			log_const_source_id((const struct log_source_const_data *)source);
	}

	zassert_equal(source_id, exp_msg->source_id, "got: %d, exp: %d",
			source_id, exp_msg->source_id);

	zassert_equal(exp_msg->data_len, desc.data_len);
	if (exp_msg->data_len <= sizeof(exp_msg->data)) {
		zassert_equal(memcmp(data, exp_msg->data, desc.data_len), 0);
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
