/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <zephyr/ztest.h>
#include <zephyr/logging/log_core.h>

void mock_log_backend_reset(const struct log_backend *backend)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	mock->msg_rec_idx = 0;
	mock->msg_proc_idx = 0;
	mock->do_check = true;
	mock->exp_drop_cnt = 0;
	mock->drop_cnt = 0;
	mock->panic = false;
}

void mock_log_backend_check_enable(const struct log_backend *backend)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	mock->do_check = true;
}

void mock_log_backend_check_disable(const struct log_backend *backend)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	mock->do_check = false;
}

void mock_log_backend_dummy_record(const struct log_backend *backend, int cnt)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	for (int i = 0; i < cnt; i++) {
		mock->exp_msgs[mock->msg_rec_idx + i].check = false;
	}

	mock->msg_rec_idx += cnt;
}

void mock_log_backend_drop_record(const struct log_backend *backend, int cnt)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	mock->exp_drop_cnt = cnt;
}

void mock_log_backend_generic_record(const struct log_backend *backend,
				     uint16_t source_id,
				     uint16_t domain_id,
				     uint8_t level,
				     log_timestamp_t timestamp,
				     const char *str,
				     uint8_t *data,
				     uint32_t data_len)
{
	if (backend->cb == NULL) {
		return;
	}

	if (IS_ENABLED(CONFIG_LOG_FRONTEND_ONLY) && timestamp != (log_timestamp_t)UINT32_MAX) {
		return;
	}

	struct mock_log_backend *mock = backend->cb->ctx;
	struct mock_log_backend_msg *exp = &mock->exp_msgs[mock->msg_rec_idx];

	exp->check = true;
	exp->timestamp = timestamp;
	exp->source_id = source_id;
	exp->domain_id = domain_id;
	exp->level = level;

	int len = strlen(str);

	__ASSERT_NO_MSG(len < sizeof(exp->str));

	memcpy(exp->str, str, len);
	exp->str[len] = 0;

	if (data_len <= sizeof(exp->data)) {
		memcpy(exp->data, data, data_len);
	}
	exp->data_len = data_len;

	mock->msg_rec_idx++;
}

void mock_log_backend_validate(const struct log_backend *backend, bool panic)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	zassert_equal(mock->exp_drop_cnt, mock->drop_cnt,
		      "Got: %u, Expected: %u", mock->drop_cnt, mock->exp_drop_cnt);
	zassert_equal(mock->msg_rec_idx, mock->msg_proc_idx,
			"%p Recored:%d, Got: %d", mock, mock->msg_rec_idx, mock->msg_proc_idx);
	zassert_equal(mock->panic, panic);

#if defined(CONFIG_LOG_MODE_DEFERRED) && \
	defined(CONFIG_LOG_PROCESS_THREAD)
	zassert_true(mock->evt_notified);
#endif
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

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	struct mock_log_backend *mock = backend->cb->ctx;
	struct mock_log_backend_msg *exp = &mock->exp_msgs[mock->msg_proc_idx];

	if (!mock->do_check) {
		return;
	}

	mock->msg_proc_idx++;

	if (!exp->check) {
		return;
	}

	zassert_equal(msg->log.hdr.timestamp, exp->timestamp,
#ifdef CONFIG_LOG_TIMESTAMP_64BIT
		      "Got: %llu, expected: %llu",
#else
		      "Got: %u, expected: %u",
#endif
		      msg->log.hdr.timestamp, exp->timestamp);
	zassert_equal(msg->log.hdr.desc.level, exp->level);
	zassert_equal(msg->log.hdr.desc.domain, exp->domain_id);

	uint32_t source_id;
	const void *source = msg->log.hdr.source;

	if (exp->level == LOG_LEVEL_INTERNAL_RAW_STRING) {
		source_id = (uintptr_t)source;
	} else if (source == NULL) {
		source_id = 0;
	} else {
		source_id = IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
		    log_dynamic_source_id((struct log_source_dynamic_data *)source) :
		    log_const_source_id((const struct log_source_const_data *)source);
	}

	zassert_equal(source_id, exp->source_id, "source_id:%p (exp: %d)",
		      source_id, exp->source_id);

	size_t len;
	uint8_t *data;
	struct cbprintf_package_desc *package_desc;

	data = log_msg_get_data(&msg->log, &len);

	zassert_equal(exp->data_len, len);
	if (exp->data_len <= sizeof(exp->data)) {
		zassert_equal(memcmp(data, exp->data, len), 0);
	}

	char str[128];
	struct test_str s = { .str = str };

	data = log_msg_get_package(&msg->log, &len);
	package_desc = (struct cbprintf_package_desc *)data;

	if (IS_ENABLED(CONFIG_LOG_MSG_APPEND_RO_STRING_LOC)) {
		/* If RO string locations are appended there is always at least 1: format string. */
		zassert_true(package_desc->ro_str_cnt > 0);
	} else {
		zassert_equal(package_desc->ro_str_cnt, 0);
	}

	len = cbpprintf(out, &s, data);
	if (len > 0) {
		str[len] = '\0';
	}

	zassert_equal(strcmp(str, exp->str), 0, "Got \"%s\", Expected:\"%s\"",
			str, exp->str);
}

static void mock_init(struct log_backend const *const backend)
{

}

static void panic(struct log_backend const *const backend)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	mock->panic = true;
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	mock->drop_cnt += cnt;
}


#if defined(CONFIG_LOG_MODE_DEFERRED) && \
	defined(CONFIG_LOG_PROCESS_THREAD)
static void notify(const struct log_backend *const backend,
		   enum log_backend_evt event,
		   union log_backend_evt_arg *arg)
{
	struct mock_log_backend *mock = backend->cb->ctx;

	mock->evt_notified = true;
}
#endif

const struct log_backend_api mock_log_backend_api = {
	.process = process,
	.panic = panic,
	.init = mock_init,
	.dropped = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ? NULL : dropped,

#if defined(CONFIG_LOG_MODE_DEFERRED) && \
	defined(CONFIG_LOG_PROCESS_THREAD)
	.notify = notify,
#endif
};
