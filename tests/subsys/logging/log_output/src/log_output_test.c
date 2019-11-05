/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test log message
 */

#include <logging/log_output.h>

#include <tc_util.h>
#include <stdbool.h>
#include <zephyr.h>
#include <ztest.h>

#define LOG_MODULE_NAME test
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static u8_t mock_buffer[512];
static u8_t log_output_buf[8];
static u32_t mock_len;

static void reset_mock_buffer(void)
{
	mock_len = 0U;
	memset(mock_buffer, 0, sizeof(mock_buffer));
}

static void setup(void)
{
	reset_mock_buffer();
}

static void teardown(void)
{

}

static int mock_output_func(u8_t *buf, size_t size, void *ctx)
{
	memcpy(&mock_buffer[mock_len], buf, size);
	mock_len += size;

	return size;
}

LOG_OUTPUT_DEFINE(log_output, mock_output_func,
		  log_output_buf, sizeof(log_output_buf));

static void validate_output_string(const char *exp)
{
	int len = strlen(exp);

	zassert_equal(len, mock_len, "Unexpected string length");
	zassert_equal(0, memcmp(exp, mock_buffer, mock_len),
			"Unxpected string");
}

static void log_output_string_varg(const struct log_output *log_output,
		       struct log_msg_ids src_level, u32_t timestamp,
		       u32_t flags, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	log_output_string(log_output, src_level, timestamp, fmt, ap, flags);

	va_end(ap);
}

void test_log_output_raw_string(void)
{
	const char *exp_str = "abc 1 3";
	const char *exp_str2 = "abc efg 3\n\r";
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_INTERNAL_RAW_STRING,
		.source_id = 0, /* not used as level indicates raw string. */
		.domain_id = 0, /* not used as level indicates raw string. */
	};

	log_output_string_varg(&log_output, src_level, 0, 0,
			       "abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str);

	reset_mock_buffer();

	/* Test adding \r after new line feed */
	log_output_string_varg(&log_output, src_level, 0, 0,
			       "abc %s %d\n", "efg", 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str2);
}

void test_log_output_string(void)
{
	const char *exp_str = STRINGIFY(LOG_MODULE_NAME) ".abc 1 3\r\n";
	const char *exp_str_lvl =
			"<dbg> " STRINGIFY(LOG_MODULE_NAME) ".abc 1 3\r\n";
	const char *exp_str_timestamp =
			"[00123456] " STRINGIFY(LOG_MODULE_NAME) ".abc 1 3\r\n";
	const char *exp_str_no_crlf = STRINGIFY(LOG_MODULE_NAME) ".abc 1 3";
	struct log_msg_ids src_level = {
		.level = LOG_LEVEL_DBG,
		.source_id = log_const_source_id(
				&LOG_ITEM_CONST_DATA(LOG_MODULE_NAME)),
		.domain_id = CONFIG_LOG_DOMAIN_ID,
	};

	log_output_string_varg(&log_output, src_level, 0,
				0 /* no flags */,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str);

	reset_mock_buffer();

	/* Test that LOG_OUTPUT_FLAG_LEVEL adds log level prefix. */
	log_output_string_varg(&log_output, src_level, 0,
				LOG_OUTPUT_FLAG_LEVEL,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str_lvl);

	reset_mock_buffer();

	/* Test that LOG_OUTPUT_FLAG_TIMESTAMP adds timestamp. */
	log_output_string_varg(&log_output, src_level, 123456,
				LOG_OUTPUT_FLAG_TIMESTAMP,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str_timestamp);

	reset_mock_buffer();

	/* Test that LOG_OUTPUT_FLAG_CRLF_NONE adds no crlf */
	log_output_string_varg(&log_output, src_level, 0,
				LOG_OUTPUT_FLAG_CRLF_NONE,
				"abc %d %d", 1, 3);
	/* test if log_output flushed correct string */
	validate_output_string(exp_str_no_crlf);
}

extern struct k_mem_slab log_msg_pool;

static bool msg_content_match(struct log_msg *msg, const char *data)
{
	int len = msg->hdr.params.hexdump.length;
	struct log_msg_cont *cont = msg->payload.ext.next;

	if (len <= LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		return memcmp(msg->payload.single.bytes, data, len) == 0;
	}

	if (memcmp(msg->payload.ext.data.bytes, data,
			LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK) != 0) {
		return false;
	}

	len -= LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
	data += LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;

	while (len > 0) {
		int cmp_len = MIN(len, HEXDUMP_BYTES_CONT_MSG);

		if (memcmp(cont->payload.bytes, data, cmp_len) != 0) {
			return false;
		}

		len -= cmp_len;
		data += cmp_len;
		cont = cont->next;
	}

	return true;
}

static void comp_message(struct log_msg *msg,
			 const char *exp_str, u32_t exp_len)
{
	LOG_OUTPUT_EXT_MSG_CONVERTER_DEFINE(converter, 8);
	struct log_msg *out_msg =
			log_output_convert_to_ext_msg(&converter, msg);

	zassert_equal(out_msg->hdr.ref_cnt, msg->hdr.ref_cnt,
			"Wrong ref number");
	zassert_equal(out_msg->hdr.timestamp, msg->hdr.timestamp,
			"Wrong timestamp");
	zassert_equal(out_msg->hdr.ids.domain_id, msg->hdr.ids.domain_id,
			"Wrong domain ID");
	zassert_equal(out_msg->hdr.ids.source_id, msg->hdr.ids.source_id,
			"Wrong source ID");
	zassert_equal(out_msg->hdr.ids.level, msg->hdr.ids.level,
			"Wrong level");

	zassert_equal(out_msg->hdr.params.hexdump.length, exp_len,
				"Wrong number of bytes: %d (expected %d)",
				out_msg->hdr.params.hexdump.length,
				exp_len);
	zassert_true(msg_content_match(out_msg, exp_str),
			"Unexpected message content");
	log_msg_put(out_msg);
}

/* Test validates correctness of function which converts log message to
 * extended log message (string formatted log message).
 */
void test_msg_to_ext_message_convert(void)
{
#define TEST_STRING "test %d"
#define EXP_STRING "test: test 5"
#define EXP_STRING_LEN strlen(EXP_STRING)

#define TEST_STRING_LONG \
	"test %d abcd %d xyz %d fff %d aaaaaaaaaa bbbbbbbb"

#define EXP_STRING_LONG "test: test 5 abcd 5 xyz 5 fff 5 aaaaaaaaaa bbbbbbbb"
#define EXP_STRING_LONG_LEN strlen(EXP_STRING_LONG)

#define TEST_HEXDUMP_STRING "metadata "
#define EXP_HEXDUMP_STRING \
	"test: metadata 30 31 32                                         " \
	"|012              "
#define EXP_HEXDUMP_STRING_LEN strlen(EXP_HEXDUMP_STRING)

	log_arg_t args[] = {5, 5, 5, 5};
	u8_t hexdump[] = {0x30, 0x31, 0x32};
	u32_t slabs_state = k_mem_slab_num_used_get(&log_msg_pool);
	struct log_msg *msg;

	/* simple, one argument message. */
	msg = log_msg_create_1(TEST_STRING, 5);
	msg->hdr.ids.domain_id = 3;
	msg->hdr.ids.source_id = log_const_source_id(
					&LOG_ITEM_CONST_DATA(LOG_MODULE_NAME));
	msg->hdr.ids.level = 2;
	msg->hdr.timestamp = 12345678;
	msg->hdr.ref_cnt = 1;

	comp_message(msg, EXP_STRING, EXP_STRING_LEN);
	log_msg_put(msg);
	/* long, multi-argument message. */
	msg = log_msg_create_n(TEST_STRING_LONG, args, ARRAY_SIZE(args));
	msg->hdr.ids.source_id = log_const_source_id(
					&LOG_ITEM_CONST_DATA(LOG_MODULE_NAME));
	msg->hdr.ids.level = 2;

	comp_message(msg, EXP_STRING_LONG, EXP_STRING_LONG_LEN);
	log_msg_put(msg);

	/* hexdump message */
	msg = log_msg_hexdump_create(TEST_HEXDUMP_STRING,
					hexdump, sizeof(hexdump));
	msg->hdr.ids.source_id = log_const_source_id(
					&LOG_ITEM_CONST_DATA(LOG_MODULE_NAME));
	msg->hdr.ids.level = 1;

	comp_message(msg, EXP_HEXDUMP_STRING, EXP_HEXDUMP_STRING_LEN);
	log_msg_put(msg);

	zassert_equal(k_mem_slab_num_used_get(&log_msg_pool), slabs_state,
			"Unexpected number of slabs in the pool");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_log_message,
		ztest_unit_test_setup_teardown(test_msg_to_ext_message_convert,
					       setup, teardown),
		ztest_unit_test_setup_teardown(test_log_output_raw_string,
					       setup, teardown),
		ztest_unit_test_setup_teardown(test_log_output_string,
					       setup, teardown)
		);
	ztest_run_test_suite(test_log_message);
}
