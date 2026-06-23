/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp_value.h>

extern struct k_heap _system_heap;

struct mp_value_api_fixture {
	struct sys_memory_stats mem_before;
};

static void *value_suite_setup(void)
{
	static struct mp_value_api_fixture fixture;

	return &fixture;
}

static void value_before(void *f)
{
	struct mp_value_api_fixture *fix = f;

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->mem_before);
}

static void value_after(void *f)
{
	struct mp_value_api_fixture *fix = f;
	struct sys_memory_stats mem_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &mem_after);
	zassert_equal(fix->mem_before.allocated_bytes, mem_after.allocated_bytes,
		      "Memory leak detected: before=%zu after=%zu", fix->mem_before.allocated_bytes,
		      mem_after.allocated_bytes);
}

ZTEST_SUITE(mp_value_api, NULL, value_suite_setup, value_before, value_after, NULL);

ZTEST(mp_value_api, test_new_values)
{
	struct mp_value *bt = mp_value_new(MP_TYPE_BOOLEAN, true);

	zassert_not_null(bt, "mp_value_new(BOOLEAN, true) returned NULL");
	zassert_equal(bt->type, MP_TYPE_BOOLEAN, "type != BOOLEAN");
	zassert_true(mp_value_get_boolean(bt), "value != true");
	mp_value_destroy(bt);

	struct mp_value *bf = mp_value_new(MP_TYPE_BOOLEAN, false);

	zassert_not_null(bf);
	zassert_false(mp_value_get_boolean(bf), "value != false");
	mp_value_destroy(bf);

	struct mp_value *iv = mp_value_new(MP_TYPE_INT, -42);

	zassert_not_null(iv, "mp_value_new(INT) returned NULL");
	zassert_equal(iv->type, MP_TYPE_INT, "type != INT");
	zassert_equal(mp_value_get_int(iv), -42, "value != -42");
	mp_value_destroy(iv);

	struct mp_value *uv = mp_value_new(MP_TYPE_UINT, 123U);

	zassert_not_null(uv);
	zassert_equal(uv->type, MP_TYPE_UINT, "type != UINT");
	zassert_equal(mp_value_get_uint(uv), 123U, "value != 123");
	mp_value_destroy(uv);

	struct mp_value *sv = mp_value_new(MP_TYPE_STRING, "hello");

	zassert_not_null(sv);
	zassert_equal(sv->type, MP_TYPE_STRING, "type != STRING");
	zassert_str_equal(mp_value_get_string(sv), "hello", "string mismatch");
	mp_value_destroy(sv);

	struct mp_value *rv = mp_value_new(MP_TYPE_INT_RANGE, 8000, 48000, 8000);

	zassert_not_null(rv);
	zassert_equal(rv->type, MP_TYPE_INT_RANGE, "type != INT_RANGE");
	zassert_equal(mp_value_get_int_range_min(rv), 8000, "min != 8000");
	zassert_equal(mp_value_get_int_range_max(rv), 48000, "max != 48000");
	zassert_equal(mp_value_get_int_range_step(rv), 8000, "step != 8000");
	mp_value_destroy(rv);

	struct mp_value *fv = mp_value_new(MP_TYPE_INT_FRACTION, 30, 1);

	zassert_not_null(fv);
	zassert_equal(fv->type, MP_TYPE_INT_FRACTION, "type != INT_FRACTION");
	zassert_equal(mp_value_get_fraction_numerator(fv), 30, "numerator != 30");
	zassert_equal(mp_value_get_fraction_denominator(fv), 1, "denominator != 1");
	mp_value_destroy(fv);

	struct mp_value *ez = mp_value_new(MP_TYPE_INT, 0);

	zassert_not_null(ez);
	zassert_equal(mp_value_get_int(ez), 0, "value != 0");
	mp_value_destroy(ez);

	struct mp_value *el = mp_value_new(MP_TYPE_LIST, NULL);

	zassert_not_null(el, "mp_value_new(LIST, NULL) returned NULL");
	zassert_equal(el->type, MP_TYPE_LIST, "type != LIST");
	zassert_equal(mp_value_list_get_size(el), 0, "list size != 0");
	zassert_true(mp_value_list_is_empty(el), "list not empty");
	mp_value_destroy(el);
}

ZTEST(mp_value_api, test_list)
{
	struct mp_value *list = mp_value_new(MP_TYPE_LIST, NULL);
	struct mp_value *item1 = mp_value_new(MP_TYPE_INT, 10);
	struct mp_value *item2 = mp_value_new(MP_TYPE_INT, 20);

	zassert_ok(mp_value_list_append(list, item1), "append item1 failed");
	zassert_ok(mp_value_list_append(list, item2), "append item2 failed");

	zassert_equal(mp_value_list_get_size(list), 2, "list size != 2");
	zassert_false(mp_value_list_is_empty(list), "list is empty");

	struct mp_value *got = mp_value_list_get(list, 0);

	zassert_not_null(got, "get(0) returned NULL");
	zassert_equal(mp_value_get_int(got), 10, "item[0] != 10");

	got = mp_value_list_get(list, 1);
	zassert_equal(mp_value_get_int(got), 20, "item[1] != 20");

	zassert_is_null(mp_value_list_get(list, 5), "get(out of range) != NULL");

	zassert_equal(mp_value_list_append(NULL, item1), -EINVAL, "append(NULL list) != -EINVAL");
	zassert_equal(mp_value_list_append(list, NULL), -EINVAL, "append(NULL value) != -EINVAL");

	mp_value_destroy(list);
}

ZTEST(mp_value_api, test_compare)
{
	struct mp_value *a10 = mp_value_new(MP_TYPE_INT, 10);
	struct mp_value *b10 = mp_value_new(MP_TYPE_INT, 10);
	struct mp_value *b20 = mp_value_new(MP_TYPE_INT, 20);
	struct mp_value *b100 = mp_value_new(MP_TYPE_INT, 100);
	struct mp_value *a100 = mp_value_new(MP_TYPE_INT, 100);

	zassert_equal(mp_value_compare(a10, b10), MP_VALUE_EQUAL, "10 == 10 failed");
	zassert_equal(mp_value_compare(a10, b20), MP_VALUE_LESS_THAN, "10 < 20 failed");
	zassert_equal(mp_value_compare(a100, b10), MP_VALUE_GREATER_THAN, "100 > 10 failed");

	mp_value_destroy(a10);
	mp_value_destroy(b10);
	mp_value_destroy(b20);
	mp_value_destroy(b100);
	mp_value_destroy(a100);
}

ZTEST(mp_value_api, test_intersect)
{
	struct mp_value *a = mp_value_new(MP_TYPE_INT, 48000);
	struct mp_value *b = mp_value_new(MP_TYPE_INT, 48000);
	struct mp_value *result = mp_value_intersect(a, b);

	zassert_not_null(result, "intersect(equal values) returned NULL");
	zassert_equal(mp_value_get_int(result), 48000, "result != 48000");
	mp_value_destroy(result);
	mp_value_destroy(a);
	mp_value_destroy(b);

	struct mp_value *range = mp_value_new(MP_TYPE_INT_RANGE, 8000, 48000, 8000);
	struct mp_value *val = mp_value_new(MP_TYPE_INT, 16000);

	result = mp_value_intersect(range, val);
	zassert_not_null(result, "intersect(value in range) returned NULL");
	mp_value_destroy(result);
	mp_value_destroy(range);
	mp_value_destroy(val);
}

ZTEST(mp_value_api, test_duplicate_and_is_primitive)
{
	struct mp_value *original = mp_value_new(MP_TYPE_INT, 999);
	struct mp_value *copy = mp_value_duplicate(original);

	zassert_not_null(copy, "duplicate returned NULL");
	zassert_true(copy != original, "duplicate == original");
	zassert_equal(mp_value_get_int(copy), 999, "duplicated value != 999");
	mp_value_destroy(original);
	mp_value_destroy(copy);

	struct mp_value *rorig = mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1);
	struct mp_value *rcopy = mp_value_duplicate(rorig);

	zassert_not_null(rcopy);
	zassert_equal(mp_value_get_int_range_min(rcopy), 1, "min != 1");
	zassert_equal(mp_value_get_int_range_max(rcopy), 100, "max != 100");
	zassert_equal(mp_value_get_int_range_step(rcopy), 1, "step != 1");
	mp_value_destroy(rorig);
	mp_value_destroy(rcopy);

	struct mp_value *iv = mp_value_new(MP_TYPE_INT, 1);

	zassert_true(mp_value_is_primitive(iv), "INT not primitive");
	mp_value_destroy(iv);

	struct mp_value *bv = mp_value_new(MP_TYPE_BOOLEAN, true);

	zassert_true(mp_value_is_primitive(bv), "BOOLEAN not primitive");
	mp_value_destroy(bv);

	struct mp_value *lv = mp_value_new(MP_TYPE_LIST, NULL);

	zassert_false(mp_value_is_primitive(lv), "LIST is primitive");
	mp_value_destroy(lv);

	struct mp_value *rv = mp_value_new(MP_TYPE_INT_RANGE, 0, 10, 1);

	zassert_false(mp_value_is_primitive(rv), "INT_RANGE is primitive");
	mp_value_destroy(rv);

	struct mp_value *ci_a = mp_value_new(MP_TYPE_INT, 10);
	struct mp_value *ci_b = mp_value_new(MP_TYPE_INT, 10);

	zassert_true(mp_value_can_intersect(ci_a, ci_b), "same-type values cannot intersect");
	mp_value_destroy(ci_a);
	mp_value_destroy(ci_b);

	struct mp_value *ci_range = mp_value_new(MP_TYPE_INT_RANGE, 0, 100, 1);
	struct mp_value *ci_val = mp_value_new(MP_TYPE_INT, 50);

	zassert_true(mp_value_can_intersect(ci_range, ci_val), "range and value cannot intersect");
	mp_value_destroy(ci_range);
	mp_value_destroy(ci_val);
}

ZTEST(mp_value_api, test_set_updates_value)
{
	struct mp_value *val = mp_value_new(MP_TYPE_INT, 10);

	mp_value_set(val, MP_TYPE_INT, 99);
	zassert_equal(mp_value_get_int(val), 99, "value != 99 after set");

	mp_value_destroy(val);
}

ZTEST(mp_value_api, test_sanity)
{
	struct mp_value *int_val = mp_value_new(MP_TYPE_INT, 42);
	struct mp_value *str_val = mp_value_new(MP_TYPE_STRING, "hello");

	zassert_equal(mp_value_compare(int_val, str_val), MP_VALUE_COMPARE_FAILED,
		      "different types compare != COMPARE_FAILED");
	mp_value_destroy(int_val);
	mp_value_destroy(str_val);

	struct mp_value *a = mp_value_new(MP_TYPE_INT, 100);
	struct mp_value *b = mp_value_new(MP_TYPE_INT, 200);
	struct mp_value *result = mp_value_intersect(a, b);

	zassert_is_null(result, "disjoint values intersect != NULL");
	mp_value_destroy(a);
	mp_value_destroy(b);
}
