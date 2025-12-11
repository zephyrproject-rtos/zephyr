/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/conn.h"
#include "mocks/hci_core.h"
#include "mocks/net_buf.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/fff.h>

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(bool, bt_le_cs_step_data_parse_func, struct bt_le_cs_subevent_step *, void *);

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	RESET_FAKE(bt_le_cs_step_data_parse_func);
	CONN_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_le_cs_step_data_parse, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test empty data buffer
 *
 *  Constraints:
 *   - buffer len set to 0
 *
 *  Expected behaviour:
 *   - Callback function is not called
 */
ZTEST(bt_le_cs_step_data_parse, test_parsing_empty_buf)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(0);

	bt_le_cs_step_data_parse(buf, bt_le_cs_step_data_parse_func, NULL);

	zassert_equal(bt_le_cs_step_data_parse_func_fake.call_count, 0);
}

/*
 *  Test malformed step data
 *
 *  Constraints:
 *   - step data with a step length going out of bounds
 *
 *  Expected behaviour:
 *   - Callback function is called once
 */
ZTEST(bt_le_cs_step_data_parse, test_parsing_invalid_length)
{
	struct net_buf_simple buf;
	uint8_t data[] = {
		0x00, 0x01, 0x01, 0x00,       /* mode 0 */
		0x03, 0x20, 0x03, 0x00, 0x11, /* mode 3 step with bad length */
	};

	bt_le_cs_step_data_parse_func_fake.return_val = true;

	net_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_le_cs_step_data_parse(&buf, bt_le_cs_step_data_parse_func, NULL);

	zassert_equal(1, bt_le_cs_step_data_parse_func_fake.call_count, "called %d",
		      bt_le_cs_step_data_parse_func_fake.call_count);
}

/*
 *  Test parsing stopped
 *
 *  Constraints:
 *   - Data contains valid step data
 *   - Callback function returns false to stop parsing
 *
 *  Expected behaviour:
 *   - Once parsing is stopped, the callback is not called anymore
 */
ZTEST(bt_le_cs_step_data_parse, test_parsing_stopped)
{
	struct net_buf_simple buf;
	uint8_t data[] = {
		0x00, 0x05, 0x01, 0x00,       /* mode 0 */
		0x01, 0x10, 0x02, 0x00, 0x11, /* mode 1 */
		0x02, 0x11, 0x02, 0x00, 0x11, /* mode 2 */
	};

	bt_le_cs_step_data_parse_func_fake.return_val = false;

	net_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_le_cs_step_data_parse(&buf, bt_le_cs_step_data_parse_func, NULL);

	zassert_equal(1, bt_le_cs_step_data_parse_func_fake.call_count, "called %d",
		      bt_le_cs_step_data_parse_func_fake.call_count);
}

struct custom_user_data {
	const uint8_t *data;
	size_t len;
};

static bool bt_le_cs_step_data_parse_func_custom_fake(struct bt_le_cs_subevent_step *step,
						      void *user_data)
{
	struct custom_user_data *ud = user_data;

	/* mode check */
	zassert_true(ud->len-- > 0);
	zassert_equal(step->mode, *ud->data);
	ud->data++;

	/* channel check */
	zassert_true(ud->len-- > 0);
	zassert_equal(step->channel, *ud->data);
	ud->data++;

	/* step data length check */
	zassert_true(ud->len-- > 0);
	zassert_equal(step->data_len, *ud->data);
	ud->data++;

	/* value check */
	zassert_true(ud->len >= step->data_len);
	zassert_mem_equal(step->data, ud->data, step->data_len);
	ud->data += step->data_len;
	ud->len -= step->data_len;

	return true;
}

/*
 *  Test parsing successfully
 *
 *  Constraints:
 *   - Data contains valid step data
 *   - Callback function returns false to stop parsing
 *
 *  Expected behaviour:
 *   - Data passed to the callback match the expected data
 */
ZTEST(bt_le_cs_step_data_parse, test_parsing_success)
{
	struct net_buf_simple buf;
	uint8_t data[] = {
		0x00, 0x05, 0x01, 0x00,       /* mode 0 */
		0x03, 0x11, 0x01, 0x11,       /* mode 3 */
		0x02, 0x12, 0x02, 0x00, 0x11, /* mode 2 */
		0x03, 0x13, 0x01, 0x11,       /* mode 3 */
		0x02, 0x14, 0x02, 0x00, 0x11, /* mode 2 */
	};

	struct custom_user_data user_data = {
		.data = data,
		.len = ARRAY_SIZE(data),
	};

	bt_le_cs_step_data_parse_func_fake.custom_fake = bt_le_cs_step_data_parse_func_custom_fake;

	net_buf_simple_init_with_data(&buf, data, ARRAY_SIZE(data));

	bt_le_cs_step_data_parse(&buf, bt_le_cs_step_data_parse_func, &user_data);

	zassert_equal(5, bt_le_cs_step_data_parse_func_fake.call_count, "called %d",
		      bt_le_cs_step_data_parse_func_fake.call_count);
}
