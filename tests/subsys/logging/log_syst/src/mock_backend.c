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

bool is_kth_bit_set(int n, int k)
{
	return ((n & (1 << (k - 1))) != 0);
}

void validate_msg(const char *type, const char *optional_flags,
		  const char *module_id, const char *sub_type,
		  const char *payload)
{
	const char *raw_data_str = "SYS-T RAW DATA: ";
	const char *output_str = test_output_buf;
	const char *syst_format_headers[4] = {type, optional_flags, module_id, sub_type};
	const char *syst_headers_name[4] = {"type", "optional_flags", "module_id", "sub_type"};

	/* Validate "SYS-T RAW DATA: " prefix in the output_str */
	zassert_mem_equal(raw_data_str, output_str, strlen(raw_data_str),
			  "Incorrect Format comparison");

	output_str += strlen(raw_data_str);

	/* Validate the headers in the SYS-T data format */
	for (int i = 0; i < ARRAY_SIZE(syst_format_headers); i++) {
		zassert_mem_equal(output_str, syst_format_headers[i],
				  strlen(syst_format_headers[i]),
				  "Incorrect Comparison of %s", syst_headers_name[i]);

		output_str = output_str+2;
	}

	/* After the headers the output_str will contain the content of optional flags.
	 * Optional flags are contained in 1st byte of data in output_str. There are 4 bits
	 * reserved for Optional flags. 1st bit = Location, 2nd Bit = payload length
	 * 3rd bit = Sys-t Checksum, 4th bit = Sys-t Timestamp. Support for validating the
	 * content based on optional flags is not supported, hence the corresponding
	 * characters are skipped. skip_bytes array contains the number of hex characters
	 * to skip in the output_str.
	 */
	size_t skip_bytes[5] = {0, 18, 4, 8, 16};

	for (int i = 1; i < ARRAY_SIZE(skip_bytes); i++) {
		if (is_kth_bit_set(strtol(optional_flags, 0, 16), i)) {
			output_str += skip_bytes[i];
		}
	}

	zassert_mem_equal(output_str, payload, strlen(payload),
			  "Incorrect Comparison of payload");

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
