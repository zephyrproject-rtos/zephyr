/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mock_backend.h"
#include <ztest.h>
#include <logging/log_core.h>
#include <logging/log_backend_std.h>

static uint32_t log_format_current = CONFIG_LOG_BACKEND_MOCK_OUTPUT_DEFAULT;
union log_msg2_generic *test_msg;

static uint8_t mock_output_buf[1];
uint8_t test_output_buf[256];
int i;

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
		test_output_buf[i] = *data;
		i++;
		data++;
		length--;

		if (i == sizeof(test_output_buf)-1) {
			i = 0;
		}
	}

	return output_length;
}

LOG_OUTPUT_DEFINE(log_output_mock, char_out, mock_output_buf, sizeof(mock_output_buf));

static void process(const struct log_backend *const backend,
		union log_msg2_generic *msg)
{
	uint32_t flags = log_backend_std_get_flags();

	test_msg = msg;

	log_format_func_t log_output_func = log_format_func_t_get(log_format_current);

	log_output_func(&log_output_mock, &msg->log, flags);
}

static void validate_string(const char *str1, const char *str2)
{
	zassert_mem_equal(str1, str2, strlen(str2), "Failed comparison");
}

void validate_msg(const char *expected_str)
{
	const char *raw_data_str = "SYS-T RAW DATA: ";
	const char *output_str = test_output_buf;

	zassert_mem_equal(raw_data_str, output_str, strlen(raw_data_str),
									"Incorrect data comparison");

	output_str += strlen(raw_data_str);

	validate_string(output_str, expected_str);
	i = 0;
}

static int format_set(const struct log_backend *const backend, uint32_t log_type)
{
	log_format_current = log_type;
	return 0;
}

static void mock_init(struct log_backend const *const backend)
{

}

const struct log_backend_api mock_log_backend_api = {
	.process = IS_ENABLED(CONFIG_LOG2) ? process : NULL,
	.init = mock_init,
	.format_set = IS_ENABLED(CONFIG_LOG1) ? NULL : format_set,
};

LOG_BACKEND_DEFINE(log_backend_mock, mock_log_backend_api, true);
