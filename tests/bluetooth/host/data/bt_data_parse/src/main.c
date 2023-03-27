/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(bool, bt_data_parse_func, struct bt_data *, void *);

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	RESET_FAKE(bt_data_parse_func);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_data_parse, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test empty data buffer
 *
 *  Constraints:
 *   - data.len set to 0
 *
 *  Expected behaviour:
 *   - Callback function is not called
 */
ZTEST(bt_data_parse, test_parsing_empty_buf)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);

	bt_data_parse(buf, bt_data_parse_func, NULL);

	zassert_equal(bt_data_parse_func_fake.call_count, 0);
}

/*
 *  Test AD Structure invalid length
 *
 *  Constraints:
 *   - AD Structure N length > number of bytes after
 *
 *  Expected behaviour:
 *   - Callback function is called N - 1 times
 */
ZTEST(bt_data_parse, test_parsing_invalid_length)
{
	struct net_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
		/* Invalid length 0xff */
		0xff, 0x03, 0x02, 0x01,                 /* AD Structure N */
		0x05, 0x04, 0x03, 0x02, 0x01, 0x00,     /* AD Structure N + 1 */
	};

	bt_data_parse_func_fake.return_val = true;

	net_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_func, NULL);

	zassert_equal(2, bt_data_parse_func_fake.call_count,
		      "called %d", bt_data_parse_func_fake.call_count);
}

/*
 *  Test early termination of the significant part
 *
 *  Constraints:
 *   - The significant part contains a sequence of N AD structures
 *   - The non-significant part extends the data with all-zero octets
 *
 *  Expected behaviour:
 *   - Callback function is called N times
 */
ZTEST(bt_data_parse, test_parsing_early_termination)
{
	struct net_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
		0x04, 0x03, 0x02, 0x01, 0x00,           /* AD Structure 3 */
		/* Non-significant part */
		0x00, 0x00, 0x00, 0x00, 0x00
	};

	bt_data_parse_func_fake.return_val = true;

	net_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_func, NULL);

	zassert_equal(3, bt_data_parse_func_fake.call_count,
		      "called %d", bt_data_parse_func_fake.call_count);
}

/*
 *  Test parsing stopped
 *
 *  Constraints:
 *   - Data contains valid AD Structures
 *   - Callback function returns false to stop parsing
 *
 *  Expected behaviour:
 *   - Once parsing is stopped, the callback is not called anymore
 */
ZTEST(bt_data_parse, test_parsing_stopped)
{
	struct net_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
	};

	bt_data_parse_func_fake.return_val = false;

	net_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_func, NULL);

	zassert_equal(1, bt_data_parse_func_fake.call_count,
		      "called %d", bt_data_parse_func_fake.call_count);
}

struct custom_fake_user_data {
	const uint8_t *data;
	size_t len;
};

static bool bt_data_parse_func_custom_fake(struct bt_data *data,
					   void *user_data)
{
	struct custom_fake_user_data *ud = user_data;

	/* length check */
	zassert_true(ud->len-- > 0);
	zassert_equal(data->data_len, *ud->data - 1);
	ud->data++;

	/* type check */
	zassert_true(ud->len-- > 0);
	zassert_equal(data->type, *ud->data);
	ud->data++;

	/* value check */
	zassert_true(ud->len >= data->data_len);
	zassert_mem_equal(data->data, ud->data, data->data_len);
	ud->data += data->data_len;
	ud->len -= data->data_len;

	return true;
}

/*
 *  Test parsing AD Data
 *
 *  Constraints:
 *   - Data contains valid AD Structures
 *   - Callback function returns false to stop parsing
 *
 *  Expected behaviour:
 *   - Data passed to the callback match the expected data
 */
ZTEST(bt_data_parse, test_parsing_success)
{
	struct net_buf_simple buf;
	uint8_t data[] = {
		/* Significant part */
		0x02, 0x01, 0x00,                       /* AD Structure 1 */
		0x03, 0x02, 0x01, 0x00,                 /* AD Structure 2 */
	};
	struct custom_fake_user_data user_data = {
		.data = data,
		.len = ARRAY_SIZE(data),
	};

	bt_data_parse_func_fake.custom_fake = bt_data_parse_func_custom_fake;

	net_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_data_parse(&buf, bt_data_parse_func, &user_data);

	zassert_equal(2, bt_data_parse_func_fake.call_count,
		      "called %d", bt_data_parse_func_fake.call_count);
}
