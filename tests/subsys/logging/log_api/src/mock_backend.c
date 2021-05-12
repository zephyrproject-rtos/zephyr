/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <ztest.h>
#include <logging/log_core.h>

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
	zassert_equal(mock->msg_rec_idx, mock->msg_proc_idx, NULL);
	zassert_equal(mock->panic, panic, NULL);
}


static void fmt_cmp(struct log_msg *msg, char *exp_str)
{
	char str[128];
	const char *fmt = msg->str;
	uint32_t nargs = log_msg_nargs_get(msg);
	log_arg_t *args = alloca(sizeof(log_arg_t)*nargs);

	for (int i = 0; i < nargs; i++) {
		args[i] = log_msg_arg_get(msg, i);
	}

	switch (log_msg_nargs_get(msg)) {
	case 0:
		snprintk(str, sizeof(str), fmt, NULL);
		break;
	case 1:
		snprintk(str, sizeof(str), fmt, args[0]);
		break;
	case 2:
		snprintk(str, sizeof(str), fmt, args[0], args[1]);
		break;
	case 3:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2]);
		break;
	case 4:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3]);
		break;
	case 5:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4]);
		break;
	case 6:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5]);
		break;
	case 7:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6]);
		break;
	case 8:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6], args[7]);
		break;
	case 9:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],  args[7],
				args[8]);
		break;
	case 10:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],  args[7],
				args[8], args[9]);
		break;
	case 11:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],  args[7],
				args[8], args[9], args[10]);
		break;
	case 12:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],  args[7],
				args[8], args[9], args[10], args[11]);
		break;
	case 13:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],  args[7],
				args[8], args[9], args[10], args[11], args[12]);
		break;
	case 14:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],  args[7],
				args[8], args[9], args[10], args[11], args[12],
				args[13]);
		break;
	case 15:
		snprintk(str, sizeof(str), fmt, args[0], args[1], args[2],
				args[3], args[4], args[5], args[6],  args[7],
				args[8], args[9], args[10], args[11], args[12],
				args[13], args[14]);
		break;
	default:
		/* Unsupported number of arguments. */
		__ASSERT_NO_MSG(true);
		break;
	}

	zassert_equal(0, strcmp(str, exp_str), NULL);
}

static void hexdump_cmp(struct log_msg *msg, uint8_t *exp_data,
			uint32_t exp_len, bool skip_data_check)
{
	uint32_t len = msg->hdr.params.hexdump.length;
	size_t offset = 0;
	size_t l;

	zassert_equal(len, exp_len, "Got: %u, expected: %u", len, exp_len);

	if (skip_data_check) {
		return;
	}

	do {
		uint8_t d;

		l = 1;
		log_msg_hexdump_data_get(msg, &d, &l, offset);
		if (l) {
			zassert_equal(d, exp_data[offset],
				"Got:%02x, Expected:%02x", d, exp_data[offset]);
		}
		offset += l;
	} while (l > 0);

	zassert_equal(offset, exp_len, NULL);

}

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	struct mock_log_backend *mock = backend->cb->ctx;
	struct mock_log_backend_msg *exp = &mock->exp_msgs[mock->msg_proc_idx];

	mock->msg_proc_idx++;

	if (!exp->check) {
		return;
	}

	zassert_equal(msg->hdr.timestamp, exp->timestamp,
		      "Got: %u, expected: %u", msg->hdr.timestamp, exp->timestamp);
	zassert_equal(msg->hdr.ids.level, exp->level, NULL);
	zassert_equal(msg->hdr.ids.source_id, exp->source_id, NULL);
	zassert_equal(msg->hdr.ids.domain_id, exp->domain_id, NULL);


	if (msg->hdr.params.generic.type == LOG_MSG_TYPE_STD) {
		zassert_equal(exp->data_len, 0, NULL);
		fmt_cmp(msg, exp->str);
	} else {
		zassert_equal(0, strcmp(msg->str, exp->str), NULL);
		hexdump_cmp(msg, exp->data, exp->data_len,
			    exp->data_len > sizeof(exp->data));
	}
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
		union log_msg2_generic *msg)
{
	struct mock_log_backend *mock = backend->cb->ctx;
	struct mock_log_backend_msg *exp = &mock->exp_msgs[mock->msg_proc_idx];

	mock->msg_proc_idx++;

	if (!exp->check) {
		return;
	}

	zassert_equal(msg->log.hdr.timestamp, exp->timestamp,
		      "Got: %u, expected: %u",
		      msg->log.hdr.timestamp, exp->timestamp);
	zassert_equal(msg->log.hdr.desc.level, exp->level, NULL);
	zassert_equal(msg->log.hdr.desc.domain, exp->domain_id, NULL);

	uint32_t source_id = IS_ENABLED(CONFIG_LOG_RUNTIME_FILTERING) ?
		log_dynamic_source_id((struct log_source_dynamic_data *)msg->log.hdr.source) :
		log_const_source_id((const struct log_source_const_data *)msg->log.hdr.source);

	zassert_equal(source_id, exp->source_id, NULL);

	size_t len;
	uint8_t *data;

	data = log_msg2_get_data(&msg->log, &len);
	zassert_equal(exp->data_len, len, NULL);
	if (exp->data_len <= sizeof(exp->data)) {
		zassert_equal(memcmp(data, exp->data, len), 0, NULL);
	}

	char str[128];
	struct test_str s = { .str = str };

	data = log_msg2_get_package(&msg->log, &len);
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

static void sync_string(const struct log_backend *const backend,
		     struct log_msg_ids src_level, uint32_t timestamp,
		     const char *fmt, va_list ap)
{
	struct mock_log_backend *mock = backend->cb->ctx;
	struct mock_log_backend_msg *exp = &mock->exp_msgs[mock->msg_proc_idx];

	mock->msg_proc_idx++;

	if (!exp->check) {
		return;
	}

	zassert_equal(timestamp, exp->timestamp,
		      "Got: %u, expected: %u", timestamp, exp->timestamp);
	zassert_equal(src_level.level, exp->level, NULL);
	zassert_equal(src_level.source_id, exp->source_id, NULL);
	zassert_equal(src_level.domain_id, exp->domain_id, NULL);
	zassert_equal(exp->data_len, 0, NULL);

	char str[128];

	vsnprintk(str, sizeof(str), fmt, ap);
	zassert_equal(0, strcmp(str, exp->str), NULL);
}

static void sync_hexdump(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t length)
{
	struct mock_log_backend *mock = backend->cb->ctx;
	struct mock_log_backend_msg *exp = &mock->exp_msgs[mock->msg_proc_idx];

	mock->msg_proc_idx++;

	if (!exp->check) {
		return;
	}

	zassert_equal(timestamp, exp->timestamp,
		      "Got: %u, expected: %u", timestamp, exp->timestamp);
	zassert_equal(src_level.level, exp->level, NULL);
	zassert_equal(src_level.source_id, exp->source_id, NULL);
	zassert_equal(src_level.domain_id, exp->domain_id, NULL);

	zassert_equal(0, strcmp(metadata, exp->str), NULL);
	zassert_equal(length, exp->data_len, NULL);
	if (length <= sizeof(exp->data)) {
		zassert_equal(memcmp(data, exp->data, length), 0, NULL);
	}
}

const struct log_backend_api mock_log_backend_api = {
	.process = IS_ENABLED(CONFIG_LOG2) ? process : NULL,
	.put = IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) ? put : NULL,
	.put_sync_string = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ?
			sync_string : NULL,
	.put_sync_hexdump = IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE) ?
			sync_hexdump : NULL,
	.panic = panic,
	.init = mock_init,
	.dropped = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : dropped,
};
