/*
 * Copyright (c) 2024 Grant Ramsay <grant.ramsay@hotmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "ssh_pkt.h"

static struct ssh_payload payload;
static uint8_t payload_buf[4096];

ZTEST(ssh_pkt, test_write_mpint)
{
	/* mpint examples from RFC 4251:
	 *   0                  00 00 00 00
	 *   9a378f9b2e332a7    00 00 00 08 09 a3 78 f9 b2 e3 32 a7
	 *   80                 00 00 00 02 00 80
	 *   -1234              00 00 00 02 ed cc
	 *   -deadbeef          00 00 00 05 ff 21 52 41 11
	 */
	static const uint8_t tc_0_data[] = { 0x00 };
	static const uint8_t tc_0_expected[] = { 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t tc_1_data[] = { 0x09, 0xa3, 0x78, 0xf9, 0xb2, 0xe3, 0x32, 0xa7 };
	static const uint8_t tc_1_expected[] = {
		0x00, 0x00, 0x00, 0x08, 0x09, 0xa3, 0x78, 0xf9, 0xb2, 0xe3, 0x32, 0xa7
	};
	static const uint8_t tc_2_data[] = { 0x80 };
	static const uint8_t tc_2_expected[] = { 0x00, 0x00, 0x00, 0x02, 0x00, 0x80};
	static const uint8_t tc_3_data[] = { 0xed, 0xcc };
	static const uint8_t tc_3_expected[] = { 0x00, 0x00, 0x00, 0x02, 0xed, 0xcc};
	static const uint8_t tc_4_data[] = { 0x21, 0x52, 0x41, 0x11 };
	static const uint8_t tc_4_expected[] = {
		0x00, 0x00, 0x00, 0x05, 0xff, 0x21, 0x52, 0x41, 0x11
	};

	/* Extra tests with leading zeros/0xFFs removed */
	static const uint8_t tc_5_data[] = { 0x00, 0x00, 0x00, 0x00, 0x80 };
	static const uint8_t tc_5_expected[] = { 0x00, 0x00, 0x00, 0x02, 0x00, 0x80};
	static const uint8_t tc_6_data[] = { 0xff, 0xff, 0xff, 0xff, 0xed, 0xcc };
	static const uint8_t tc_6_expected[] = { 0x00, 0x00, 0x00, 0x02, 0xed, 0xcc};

	static const struct test_case {
		const uint8_t *data;
		uint8_t data_len;
		const uint8_t *expected;
		uint8_t expected_len;
		bool is_signed;
	} test_cases[] = {
		{
			.data = tc_0_data, .data_len = sizeof(tc_0_data),
			.expected = tc_0_expected, .expected_len = sizeof(tc_0_expected), false
		},
		{
			.data = tc_1_data, .data_len = sizeof(tc_1_data),
			.expected = tc_1_expected, .expected_len = sizeof(tc_1_expected), false
		},
		{
			.data = tc_2_data, .data_len = sizeof(tc_2_data),
			.expected = tc_2_expected, .expected_len = sizeof(tc_2_expected), false
		},
		{
			.data = tc_3_data, .data_len = sizeof(tc_3_data),
			.expected = tc_3_expected, .expected_len = sizeof(tc_3_expected), true
		},
		{
			.data = tc_4_data, .data_len = sizeof(tc_4_data),
			.expected = tc_4_expected, .expected_len = sizeof(tc_4_expected), true
		},
		{
			.data = tc_5_data, .data_len = sizeof(tc_5_data),
			.expected = tc_5_expected, .expected_len = sizeof(tc_5_expected), false
		},
		{
			.data = tc_6_data, .data_len = sizeof(tc_6_data),
			.expected = tc_6_expected, .expected_len = sizeof(tc_6_expected), true
		},
	};

	uint8_t data[32];

	for (unsigned int i = 0; i < ARRAY_SIZE(test_cases); i++) {
		const struct test_case *tc = &test_cases[i];
		struct ssh_payload null_payload = {
			.size = UINT32_MAX
		};

		/* Check success with exact length buffer */
		payload.size = tc->expected_len;
		payload.len = 0;
		zassert_true(ssh_payload_write_mpint(
			&payload, tc->data, tc->data_len, tc->is_signed));
		zassert_equal(payload.len, tc->expected_len);
		zassert_mem_equal(payload.data, tc->expected, tc->expected_len);

		/* Check NULL payload gives output length */
		zassert_true(ssh_payload_write_mpint(
			&null_payload, tc->data, tc->data_len, tc->is_signed));
		zassert_equal(null_payload.len, tc->expected_len);

		/* Check payload len too small */
		payload.size = tc->expected_len - 1;
		payload.len = 0;
		zassert_false(ssh_payload_write_mpint(
			&payload, tc->data, tc->data_len, tc->is_signed));
	}

	/* Test wrap */
	memset(data, 0x12, sizeof(data));
	payload = (struct ssh_payload) {.size = UINT32_MAX, .len = UINT32_MAX};
	zassert_false(ssh_payload_write_mpint(&payload, data, sizeof(data), false));
}

ZTEST(ssh_pkt, test_payload_list_iter)
{
#define TEST_LIST "name1,name2,name3"
	struct ssh_string input = {
		.data = (uint8_t *)TEST_LIST,
		.len = sizeof(TEST_LIST) - 1,
	};
	struct ssh_string name;
	struct ssh_payload name_list = {
		.size = input.len,
		.len = 0,
		.data = (void *)input.data,
	};
	static const struct expecting {
		const char *name;
		size_t len;
	} expected_names[] = {
		{ .name = "name1", .len = 5 },
		{ .name = "name2", .len = 5 },
		{ .name = "name3", .len = 5 },
	};

	while (ssh_payload_name_list_iter(&name_list, &name)) {
		zassert_true(name.len > 0);
		zassert_equal(name.len, expected_names[name.data[4] - '1'].len);
	}
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	payload = (struct ssh_payload) {
		.size = sizeof(payload_buf),
		.len = 0,
		.data = payload_buf
	};

	memset(&payload_buf, 0, sizeof(payload_buf));
}

ZTEST_SUITE(ssh_pkt, NULL, NULL, before, NULL, NULL);
