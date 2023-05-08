/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/linear_range.h>

/*
 * +-----+-----+
 * | Val | Idx |
 * +-----+-----+
 * | -10 | 0   |
 * | -5  | 1   |
 * +-----+-----+
 * | 0   | 2   |
 * | 1   | 3   |
 * +-----+-----+
 * | 100 | 4   |
 * | 130 | 5   |
 * | 160 | 6   |
 * | 190 | 7   |
 * | 220 | 8   |
 * | 250 | 9   |
 * | 290 | 10  |
 * +-----+-----+
 * | 400 | 11  |
 * | 400 | 12  |
 * +-----+-----+
 */
static const struct linear_range r[] = {
	LINEAR_RANGE_INIT(-10, 5U,  0U,  1U),
	LINEAR_RANGE_INIT(0,   1U,  2U,  3U),
	LINEAR_RANGE_INIT(100, 30U, 4U,  10U),
	LINEAR_RANGE_INIT(400, 0U,  11U, 12U),
};

static const size_t r_cnt = ARRAY_SIZE(r);

ZTEST(linear_range, test_linear_range_init)
{
	zassert_equal(r[0].min, -10);
	zassert_equal(r[0].step, 5U);
	zassert_equal(r[0].min_idx, 0U);
	zassert_equal(r[0].max_idx, 1U);
}

ZTEST(linear_range, test_linear_range_values_count)
{
	zassert_equal(linear_range_values_count(&r[0]), 2U);
	zassert_equal(linear_range_values_count(&r[1]), 2U);
	zassert_equal(linear_range_values_count(&r[2]), 7U);
	zassert_equal(linear_range_values_count(&r[3]), 2U);

	zassert_equal(linear_range_group_values_count(r, r_cnt), 13U);
}

ZTEST(linear_range, test_linear_range_get_max_value)
{
	zassert_equal(linear_range_get_max_value(&r[0]), -5);
	zassert_equal(linear_range_get_max_value(&r[1]), 1);
	zassert_equal(linear_range_get_max_value(&r[2]), 280);
	zassert_equal(linear_range_get_max_value(&r[3]), 400);
}

ZTEST(linear_range, test_linear_range_get_value)
{
	int ret;
	int32_t val;

	ret = linear_range_get_value(&r[0], 0U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, -10);

	ret = linear_range_get_value(&r[0], 1U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, -5);

	ret = linear_range_get_value(&r[1], 2U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 0);

	ret = linear_range_get_value(&r[1], 3U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 1);

	ret = linear_range_get_value(&r[2], 4U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 100);

	ret = linear_range_get_value(&r[2], 5U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 130);

	ret = linear_range_get_value(&r[2], 6U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 160);

	ret = linear_range_get_value(&r[2], 7U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 190);

	ret = linear_range_get_value(&r[2], 8U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 220);

	ret = linear_range_get_value(&r[2], 9U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 250);

	ret = linear_range_get_value(&r[2], 10U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 280);

	ret = linear_range_get_value(&r[3], 11U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 400);

	ret = linear_range_get_value(&r[3], 12U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 400);

	ret = linear_range_get_value(&r[1], 13U, &val);
	zassert_equal(ret, -EINVAL);

	ret = linear_range_group_get_value(r, r_cnt, 0U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, -10);

	ret = linear_range_group_get_value(r, r_cnt, 1U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, -5);

	ret = linear_range_group_get_value(r, r_cnt, 2U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 0);

	ret = linear_range_group_get_value(r, r_cnt, 3U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 1);

	ret = linear_range_group_get_value(r, r_cnt, 4U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 100);

	ret = linear_range_group_get_value(r, r_cnt, 5U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 130);

	ret = linear_range_group_get_value(r, r_cnt, 6U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 160);

	ret = linear_range_group_get_value(r, r_cnt, 7U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 190);

	ret = linear_range_group_get_value(r, r_cnt, 8U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 220);

	ret = linear_range_group_get_value(r, r_cnt, 9U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 250);

	ret = linear_range_group_get_value(r, r_cnt, 10U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 280);

	ret = linear_range_group_get_value(r, r_cnt, 11U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 400);

	ret = linear_range_group_get_value(r, r_cnt, 12U, &val);
	zassert_equal(ret, 0);
	zassert_equal(val, 400);
}

ZTEST(linear_range, test_linear_range_get_index)
{
	int ret;
	uint16_t idx;

	/* negative */
	ret = linear_range_get_index(&r[0], -10, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 0U);

	ret = linear_range_get_index(&r[0], -7, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 1U);

	/* out of range (< min, > max) */
	ret = linear_range_get_index(&r[1], -1, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 2U);

	ret = linear_range_get_index(&r[1], 2, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 3U);

	/* range limits */
	ret = linear_range_get_index(&r[2], 100, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 4U);

	ret = linear_range_get_index(&r[2], 280, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 10U);

	/* rounding: 120->130 (5) */
	ret = linear_range_get_index(&r[2], 120, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 5U);

	/* always minimum index in constant ranges */
	ret = linear_range_get_index(&r[3], 400, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 11U);

	/* group */
	ret = linear_range_group_get_index(r, r_cnt, -20, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 0U);

	ret = linear_range_group_get_index(r, r_cnt, -6, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 1U);

	ret = linear_range_group_get_index(r, r_cnt, 0, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 2U);

	ret = linear_range_group_get_index(r, r_cnt, 50, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 4U);

	ret = linear_range_group_get_index(r, r_cnt, 200, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 8U);

	ret = linear_range_group_get_index(r, r_cnt, 400, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 11U);
}

ZTEST(linear_range, test_linear_range_get_win_index)
{
	int ret;
	uint16_t idx;

	/* negative */
	ret = linear_range_get_win_index(&r[0], -10, -6, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 0U);

	ret = linear_range_get_win_index(&r[0], -7, -5, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 1U);

	/* no intersection */
	ret = linear_range_get_win_index(&r[0], -20, -15, &idx);
	zassert_equal(ret, -EINVAL);

	ret = linear_range_get_win_index(&r[0], -4, -3, &idx);
	zassert_equal(ret, -EINVAL);

	/* out of range, partial intersection (< min, > max) */
	ret = linear_range_get_win_index(&r[1], -1, 0, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 2U);

	ret = linear_range_get_win_index(&r[1], 1, 2, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 3U);

	/* min/max equal */
	ret = linear_range_get_win_index(&r[2], 100, 100, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 4U);

	/* always minimum index that satisfies >= min */
	ret = linear_range_get_win_index(&r[2], 100, 180, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 4U);

	/* rounding: 120->130, maximum < 130 */
	ret = linear_range_get_win_index(&r[2], 120, 140, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 5U);

	/* rounding: 120->130, maximum > 125 (range too narrow) */
	ret = linear_range_get_win_index(&r[2], 120, 125, &idx);
	zassert_equal(ret, -EINVAL);

	ret = linear_range_group_get_win_index(r, r_cnt, 120, 125, &idx);
	zassert_equal(ret, -EINVAL);

	/* always minimum index in constant ranges */
	ret = linear_range_get_win_index(&r[3], 400, 400, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 11U);

	/* group */
	ret = linear_range_group_get_win_index(r, r_cnt, -10, -8, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 0U);

	ret = linear_range_group_get_win_index(r, r_cnt, 0, 1, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 2U);

	ret = linear_range_group_get_win_index(r, r_cnt, 1, 120, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 3U);

	ret = linear_range_group_get_win_index(r, r_cnt, 120, 140, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 5U);

	ret = linear_range_group_get_win_index(r, r_cnt, 140, 400, &idx);
	zassert_equal(ret, -ERANGE);
	zassert_equal(idx, 10U);

	ret = linear_range_group_get_win_index(r, r_cnt, 400, 400, &idx);
	zassert_equal(ret, 0);
	zassert_equal(idx, 11U);

	ret = linear_range_group_get_win_index(r, r_cnt, 300, 310, &idx);
	zassert_equal(ret, -EINVAL);
}

ZTEST_SUITE(linear_range, NULL, NULL, NULL, NULL, NULL);
