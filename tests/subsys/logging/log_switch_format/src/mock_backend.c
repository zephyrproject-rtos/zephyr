/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <zephyr/ztest.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/logging/log_backend_std.h>
#include <stdlib.h>

static uint32_t log_format_current = CONFIG_LOG_BACKEND_MOCK_OUTPUT_DEFAULT;
union log_msg_generic *test_msg;

extern struct k_sem my_sem;
static uint8_t mock_output_buf[1];
uint8_t test_output_buf[256];
int pos_in_buf;

/**
 * @brief This function takes in the characters and copies
 * them in the temporary buffer.
 *
 * @return No. of characters copied to the temporary buffer.
 */
static int char_out(uint8_t *data, size_t length, void *ctx)
{
	ARG_UNUSED(ctx);
	size_t output_length = length;

	while (length > 0) {
		test_output_buf[pos_in_buf] = *data;
		pos_in_buf++;
		if (*data == '\n') {
			k_sem_give(&my_sem);
		}
		data++;
		length--;

		zassert_not_equal(pos_in_buf, sizeof(test_output_buf)-1,
				  "Increase the size of test_output_buf");
	}
	return output_length;
}

LOG_OUTPUT_DEFINE(log_output_mock, char_out, mock_output_buf, sizeof(mock_output_buf));

static void process(const struct log_backend *const backend,
		union log_msg_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	test_msg = msg;

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_mock, &msg->log, flags);
}

int remove_timestamp(const char *raw_string, int timestamp_start)
{
	int timestamp_end = timestamp_start;
	int count = 1;

	while (count > 0) {

		char c = raw_string[++timestamp_end];

		if (c == '[') {
			count++;
		} else if (c == '<') {
			count--;
		}
	}
	return timestamp_end;
}

void validate_log_type(const char *raw_data_str, uint32_t log_type)
{
	const char *output_str = test_output_buf;

	if (log_type == LOG_OUTPUT_TEXT) {

		int pos = remove_timestamp(output_str, 0);

		/* Skip comparing Timestamp */
		output_str = output_str+pos;
	}

	/* Validate raw_data_str prefix in the output_str */
	zassert_mem_equal(raw_data_str, output_str, strlen(raw_data_str),
			  "Incorrect Format comparison %s vs %s", output_str, raw_data_str);

	memset(test_output_buf, 0, sizeof(test_output_buf));
	pos_in_buf = 0;
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void mock_init(struct log_backend const *const backend)
{

}

static void panic(struct log_backend const *const backend)
{

}

const struct log_backend_api mock_log_backend_api = {
	.process = process,
	.init = mock_init,
	.format_set = format_set,
	.panic = panic
};

LOG_BACKEND_DEFINE(log_backend_mock, mock_log_backend_api, true);
