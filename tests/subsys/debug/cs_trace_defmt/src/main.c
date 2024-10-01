/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/debug/coresight/cs_trace_defmt.h>

static const uint8_t *exp_data[3];
static size_t exp_len[3];
static uint8_t exp_id[3];
static uint8_t cb_cnt;

static void callback(uint32_t id, const uint8_t *data, size_t len)
{
	zassert_equal(exp_id[cb_cnt], id, NULL);
	zassert_equal(len, exp_len[cb_cnt], NULL);
	zassert_equal(memcmp(data, exp_data[cb_cnt], len), 0, NULL);
	cb_cnt++;
}

ZTEST(coresight_trace_deformatter_test, test_err_check)
{
	int err;
	uint8_t data[16];

	err = cs_trace_defmt_init(callback);
	zassert_equal(err, 0);

	err = cs_trace_defmt_process(data, 15);
	zassert_equal(err, -EINVAL);

	err = cs_trace_defmt_process(data, 17);
	zassert_equal(err, -EINVAL);
}

ZTEST(coresight_trace_deformatter_test, test_basic)
{
	int err;
	static const uint8_t id = 0x25;
	static const uint8_t data1[] = {/* First byte has ID, byte 2 has LSB bit set */
					(id << 1) | 1,
					0x6,
					0x0 /*LSB bit in aux */,
					0xe,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0,
					0x2};

	static const uint8_t exp_data1[] = {0x6, 0x1, 0xe, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	cb_cnt = 0;
	exp_id[0] = id;
	exp_data[0] = exp_data1;
	exp_len[0] = 14;

	cs_trace_defmt_init(callback);
	err = cs_trace_defmt_process(data1, 16);
	zassert_equal(err, 0);
	zassert_equal(cb_cnt, 1);
}

ZTEST(coresight_trace_deformatter_test, test_basic2)
{
	int err;
	static const uint8_t data1[] = {0x07, 0xAA, 0xA6, 0xA7, 0x2B, 0xA8, 0x54, 0x52,
					0x52, 0x54, 0x07, 0xCA, 0xC6, 0xC7, 0xC8, 0x1C};

	static const uint8_t exp_data1[] = {0xAA, 0xA6, 0xA7, 0xA8};
	static const uint8_t exp_data2[] = {0x55, 0x52, 0x53, 0x54};
	static const uint8_t exp_data3[] = {0xCA, 0xC6, 0xC7, 0xC8};

	cb_cnt = 0;
	exp_id[0] = 0x3;
	exp_data[0] = exp_data1;
	exp_len[0] = sizeof(exp_data1);
	exp_id[1] = 0x15;
	exp_data[1] = exp_data2;
	exp_len[1] = sizeof(exp_data2);
	exp_id[2] = 0x3;
	exp_data[2] = exp_data3;
	exp_len[2] = sizeof(exp_data3);

	cs_trace_defmt_init(callback);
	err = cs_trace_defmt_process(data1, 16);
	zassert_equal(err, 0);
	zassert_equal(cb_cnt, 3);
}

ZTEST_SUITE(coresight_trace_deformatter_test, NULL, NULL, NULL, NULL, NULL);
