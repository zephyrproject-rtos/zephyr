/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/mp_caps.h>
#include <zephyr/mp/mp_structure.h>
#include <zephyr/mp/mp_value.h>

#include "mp_test_helpers.h"

extern struct k_heap _system_heap;

struct mp_structure_api_fixture {
	struct sys_memory_stats mem_before;
};

static void *structure_suite_setup(void)
{
	static struct mp_structure_api_fixture fixture;

	return &fixture;
}

static void structure_before(void *f)
{
	struct mp_structure_api_fixture *fix = f;

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->mem_before);
}

static void structure_after(void *f)
{
	struct mp_structure_api_fixture *fix = f;
	struct sys_memory_stats mem_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &mem_after);
	zassert_equal(fix->mem_before.allocated_bytes, mem_after.allocated_bytes,
		      "Memory leak detected: before=%zu after=%zu", fix->mem_before.allocated_bytes,
		      mem_after.allocated_bytes);
}

ZTEST_SUITE(mp_structure_api, NULL, structure_suite_setup, structure_before, structure_after, NULL);

/* Field IDs used by intersection tests */
enum test_field {
	TEST_BOOL = 0,
	TEST_INT,
	TEST_UINT,
	TEST_STRING,
	TEST_FRACTION,
	TEST_RANGE_INT,
	TEST_RANGE_UINT,
	TEST_INT_FRACTION_RANGE,
	TEST_UINT_FRACTION_RANGE,
	TEST_LIST,
};

ZTEST(mp_structure_api, test_new)
{
	struct mp_structure *s =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT, 48000,
				 MP_CAPS_BITWIDTH, MP_TYPE_INT, 16, MP_STRUCTURE_END);

	zassert_not_null(s, "mp_structure_new returned NULL");
	zassert_equal(s->media_type_id, MP_MEDIA_AUDIO_PCM, "media_type_id mismatch");

	zassert_ok(mp_structure_remove_field(s, MP_CAPS_SAMPLE_RATE), "remove_field failed");
	zassert_is_null(mp_structure_get_value(s, MP_CAPS_SAMPLE_RATE),
			"removed field still found");
	zassert_not_null(mp_structure_get_value(s, MP_CAPS_BITWIDTH), "non-removed field missing");

	mp_structure_clear(s);
	zassert_is_null(mp_structure_get_value(s, MP_CAPS_BITWIDTH), "field found after clear");
	mp_structure_destroy(s);

	struct mp_structure si;

	zassert_ok(mp_structure_init(&si, MP_MEDIA_AUDIO_PCM));

	struct mp_value *appended = mp_value_new(MP_TYPE_INT, 44100);

	zassert_ok(mp_structure_append(&si, MP_CAPS_SAMPLE_RATE, appended), "append failed");

	struct mp_value *retrieved = mp_structure_get_value(&si, MP_CAPS_SAMPLE_RATE);

	zassert_not_null(retrieved, "appended field not found");
	zassert_equal(mp_value_get_int(retrieved), 44100, "retrieved value != 44100");

	struct mp_value *dup_val = mp_value_new(MP_TYPE_INT, 0);

	zassert_equal(mp_structure_append(&si, MP_CAPS_SAMPLE_RATE, dup_val), -EEXIST,
		      "duplicate field != -EEXIST");
	mp_value_destroy(dup_val);

	zassert_equal(mp_structure_init(NULL, MP_MEDIA_AUDIO_PCM), -EINVAL,
		      "init(NULL) != -EINVAL");
	zassert_equal(mp_structure_append(NULL, MP_CAPS_SAMPLE_RATE, appended), -EINVAL,
		      "append(NULL struct) != -EINVAL");
	zassert_equal(mp_structure_append(&si, MP_CAPS_BITWIDTH, NULL), -EINVAL,
		      "append(NULL value) != -EINVAL");
	mp_structure_clear(&si);
}

ZTEST(mp_structure_api, test_is_fixed_fixate_duplicate)
{
	struct mp_structure *fixed_s =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT, 48000,
				 MP_CAPS_BITWIDTH, MP_TYPE_INT, 16, MP_STRUCTURE_END);

	zassert_true(mp_structure_is_fixed(fixed_s), "structure not fixed");

	struct mp_structure *dup = mp_structure_duplicate(fixed_s);

	zassert_not_null(dup, "duplicate returned NULL");
	zassert_true(dup != fixed_s, "duplicate == original");
	zassert_equal(dup->media_type_id, fixed_s->media_type_id, "media_type_id mismatch");
	zassert_equal(mp_value_get_int(mp_structure_get_value(dup, MP_CAPS_SAMPLE_RATE)), 48000,
		      "duplicated value != 48000");
	mp_structure_destroy(dup);
	mp_structure_destroy(fixed_s);

	struct mp_structure *range_s =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT_RANGE, 8000,
				 48000, 8000, MP_STRUCTURE_END);

	zassert_false(mp_structure_is_fixed(range_s), "range structure is fixed");

	struct mp_structure *fixated = mp_structure_fixate(range_s);

	zassert_not_null(fixated, "fixate returned NULL");
	zassert_true(mp_structure_is_fixed(fixated), "fixated structure not fixed");
	mp_structure_destroy(range_s);
	mp_structure_destroy(fixated);
}

/* Intersect two structures where all fields are primitive (fixed) values. */
ZTEST(mp_structure_api, test_intersect_primitive)
{
	struct mp_structure *s1 = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, TEST_BOOL, MP_TYPE_BOOLEAN, true, TEST_INT, MP_TYPE_INT, -123,
		TEST_UINT, MP_TYPE_UINT, 123, TEST_STRING, MP_TYPE_STRING, "xRGB", TEST_FRACTION,
		MP_TYPE_INT_FRACTION, 30, 1, MP_STRUCTURE_END);
	struct mp_structure *s2 = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, TEST_BOOL, MP_TYPE_BOOLEAN, true, TEST_INT, MP_TYPE_INT, -123,
		TEST_UINT, MP_TYPE_UINT, 123, TEST_STRING, MP_TYPE_STRING, "xRGB", TEST_FRACTION,
		MP_TYPE_INT_FRACTION, 30, 1, MP_STRUCTURE_END);

	zassert_true(mp_structure_can_intersect(s1, s2), "identical structures cannot intersect");

	struct mp_structure *result = mp_structure_intersect(s1, s2);

	zassert_not_null(result, "intersection returned NULL");

	struct mp_value *v = mp_structure_get_value(result, TEST_BOOL);

	validate_boolean_value(v, true);

	v = mp_structure_get_value(result, TEST_INT);
	validate_int_value(v, -123);

	v = mp_structure_get_value(result, TEST_UINT);
	validate_uint_value(v, 123);

	v = mp_structure_get_value(result, TEST_STRING);
	validate_string_value(v, "xRGB");

	v = mp_structure_get_value(result, TEST_FRACTION);
	validate_int_fraction_value(v, 30, 1);

	mp_structure_destroy(s1);
	mp_structure_destroy(s2);
	mp_structure_destroy(result);
}

/* Intersect a structure with an INT_RANGE field against a structure with a fixed INT value. */
ZTEST(mp_structure_api, test_intersect_int_range)
{
	struct {
		int value;
		int expected;
	} test_cases[] = {
		{INT_MIN, INT_MIN},
		{INT_MAX, INT_MAX},
		{(INT_MIN + INT_MAX) / 2, (INT_MIN + INT_MAX) / 2},
	};

	struct mp_structure *s_range =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_INT, MP_TYPE_INT_RANGE, INT_MIN,
				 INT_MAX, 1, MP_STRUCTURE_END);

	zassert_not_null(s_range, "range structure alloc failed");

	for (int i = 0; i < ARRAY_SIZE(test_cases); i++) {
		struct mp_structure *s_val =
			mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_INT, MP_TYPE_INT,
					 test_cases[i].value, MP_STRUCTURE_END);
		zassert_not_null(s_val, "value structure alloc failed");

		struct mp_structure *result = mp_structure_intersect(s_range, s_val);

		zassert_not_null(result, "intersection returned NULL");

		struct mp_value *v = mp_structure_get_value(result, TEST_RANGE_INT);

		validate_int_value(v, test_cases[i].expected);

		mp_structure_destroy(s_val);
		mp_structure_destroy(result);
	}

	mp_structure_destroy(s_range);
}

/* Intersect a structure with a UINT_RANGE field against a structure with a fixed UINT value. */
ZTEST(mp_structure_api, test_intersect_uint_range)
{
	struct {
		unsigned int expected;
		const char *description;
	} test_cases[] = {
		{0, "Zero value"},
		{UINT32_MAX, "Maximum value"},
		{UINT32_MAX / 2, "Mid-range value"},
	};

	struct mp_structure *s_range =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_UINT, MP_TYPE_UINT_RANGE, 0,
				 UINT32_MAX, 1, MP_STRUCTURE_END);

	zassert_not_null(s_range, "range structure alloc failed");

	for (int i = 0; i < ARRAY_SIZE(test_cases); i++) {
		struct mp_structure *s_val =
			mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_UINT, MP_TYPE_UINT,
					 test_cases[i].expected, MP_STRUCTURE_END);
		zassert_not_null(s_val, "value structure alloc failed for: %s",
				 test_cases[i].description);

		struct mp_structure *result = mp_structure_intersect(s_range, s_val);

		zassert_not_null(result, "intersection returned NULL for: %s",
				 test_cases[i].description);

		struct mp_value *v = mp_structure_get_value(result, TEST_RANGE_UINT);

		validate_uint_value(v, test_cases[i].expected);

		mp_structure_destroy(s_val);
		mp_structure_destroy(result);
	}

	mp_structure_destroy(s_range);
}

/* Intersect structures with fraction and fraction-range fields. */
ZTEST(mp_structure_api, test_intersect_fraction_range)
{
	struct mp_structure *s_frac =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_INT_FRACTION_RANGE, MP_TYPE_INT_FRACTION,
				 1, INT_MIN, MP_STRUCTURE_END);
	struct mp_structure *s_frac_range = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, TEST_INT_FRACTION_RANGE, MP_TYPE_INT_FRACTION_RANGE, 1, INT_MIN,
		INT_MAX, 1, 1, 1, MP_STRUCTURE_END);

	struct mp_structure *result = mp_structure_intersect(s_frac_range, s_frac_range);

	zassert_not_null(result, "range & range intersection returned NULL");
	struct mp_value *v = mp_structure_get_value(result, TEST_INT_FRACTION_RANGE);

	validate_fraction_int_range(v, 1, INT_MIN, INT_MAX, 1, 1, 1);
	mp_structure_destroy(result);

	result = mp_structure_intersect(s_frac, s_frac_range);
	zassert_not_null(result, "fixed & range intersection returned NULL");
	v = mp_structure_get_value(result, TEST_INT_FRACTION_RANGE);
	validate_int_fraction_value(v, 1, INT_MIN);
	mp_structure_destroy(result);

	mp_structure_destroy(s_frac);
	mp_structure_destroy(s_frac_range);

	struct mp_structure *s_ufrac =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_UINT_FRACTION_RANGE,
				 MP_TYPE_UINT_FRACTION, 1, UINT32_MAX, MP_STRUCTURE_END);
	struct mp_structure *s_ufrac_range = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, TEST_UINT_FRACTION_RANGE, MP_TYPE_UINT_FRACTION_RANGE, 1,
		UINT32_MAX, UINT32_MAX, 1, 1, 1, MP_STRUCTURE_END);

	result = mp_structure_intersect(s_ufrac_range, s_ufrac_range);
	zassert_not_null(result, "uint frac range & range returned NULL");
	v = mp_structure_get_value(result, TEST_UINT_FRACTION_RANGE);
	validate_uint_fraction_range(v, 1, UINT32_MAX, UINT32_MAX, 1, 1, 1);
	mp_structure_destroy(result);

	result = mp_structure_intersect(s_ufrac, s_ufrac_range);
	zassert_not_null(result, "uint fixed & range returned NULL");
	v = mp_structure_get_value(result, TEST_UINT_FRACTION_RANGE);
	validate_uint_fraction_value(v, 1, UINT32_MAX);
	mp_structure_destroy(result);

	mp_structure_destroy(s_ufrac);
	mp_structure_destroy(s_ufrac_range);
}

/* Intersect structures with LIST fields. */
ZTEST(mp_structure_api, test_intersect_list)
{
	struct mp_structure *s1 = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, TEST_LIST, MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15),
			     mp_value_new(MP_TYPE_UINT, 30),
			     mp_value_new(MP_TYPE_INT_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL), NULL),
		MP_STRUCTURE_END);
	struct mp_structure *s2 = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, TEST_LIST, MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_UINT, 30),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_INT, 15), NULL),
		MP_STRUCTURE_END);

	struct mp_structure *result = mp_structure_intersect(s1, s2);

	zassert_not_null(result, "list intersection returned NULL");

	struct mp_value *list = mp_structure_get_value(result, TEST_LIST);

	validate_list_value_type_and_size(list, 7);

	struct mp_value *item = mp_value_list_get(list, 0);

	validate_int_value(item, 15);

	item = mp_value_list_get(list, 1);
	validate_uint_value(item, 30);

	item = mp_value_list_get(list, 2);
	validate_int_fraction_value(item, 15, 1);

	item = mp_value_list_get(list, 3);
	validate_int_range_value(item, 1, 100, 1);

	item = mp_value_list_get(list, 4);
	validate_fraction_int_range(item, 100, 1, 60, 1, 1, 1);

	item = mp_value_list_get(list, 5);
	validate_string_value(item, "RGB");

	item = mp_value_list_get(list, 6);
	validate_list_value_type_and_size(item, 1);
	validate_int_value(mp_value_list_get(item, 0), 15);

	mp_structure_destroy(s1);
	mp_structure_destroy(s2);
	mp_structure_destroy(result);
}

ZTEST(mp_structure_api, test_intersect_asymmetric_fields)
{
	struct mp_structure *s1 = mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_INT, MP_TYPE_INT, -42,
						   TEST_UINT, MP_TYPE_UINT, 100U, MP_STRUCTURE_END);
	struct mp_structure *s2 =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, TEST_UINT, MP_TYPE_UINT, 100U, TEST_STRING,
				 MP_TYPE_STRING, "hello", MP_STRUCTURE_END);

	zassert_true(mp_structure_can_intersect(s1, s2), "asymmetric structures cannot intersect");

	struct mp_structure *result = mp_structure_intersect(s1, s2);

	zassert_not_null(result, "intersection returned NULL");

	zassert_equal(mp_structure_len(result), 3, "result field count != 3");

	struct mp_value *v = mp_structure_get_value(result, TEST_INT);

	validate_int_value(v, -42);

	v = mp_structure_get_value(result, TEST_UINT);
	validate_uint_value(v, 100U);

	v = mp_structure_get_value(result, TEST_STRING);
	validate_string_value(v, "hello");

	mp_structure_destroy(s1);
	mp_structure_destroy(s2);
	mp_structure_destroy(result);
}

ZTEST(mp_structure_api, test_cannot_intersect)
{
	struct mp_structure *s_sample_int = mp_structure_new(
		MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT, 48000, MP_STRUCTURE_END);
	struct mp_structure *s_bw = mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_BITWIDTH,
						     MP_TYPE_INT, 16, MP_STRUCTURE_END);
	struct mp_structure *s_low =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT_RANGE, 8000,
				 16000, 8000, MP_STRUCTURE_END);

	zassert_false(mp_structure_can_intersect(s_sample_int, NULL),
		      "can_intersect(s, NULL) should return false");
	zassert_false(mp_structure_can_intersect(NULL, NULL),
		      "can_intersect(NULL, NULL) should return false");
	zassert_is_null(mp_structure_intersect(s_sample_int, NULL),
			"intersect(s, NULL) should return NULL");
	zassert_is_null(mp_structure_intersect(NULL, NULL),
			"intersect(NULL, NULL) should return NULL");

	zassert_false(mp_structure_can_intersect(s_sample_int, s_bw),
		      "structures with no common field should not intersect");
	zassert_is_null(mp_structure_intersect(s_sample_int, s_bw),
			"intersect with no common field should return NULL");

	zassert_false(mp_structure_can_intersect(s_low, s_sample_int),
		      "out-of-range value should not intersect");
	zassert_is_null(mp_structure_intersect(s_low, s_sample_int),
			"intersect with incompatible field value should return NULL");

	mp_structure_destroy(s_sample_int);
	mp_structure_destroy(s_bw);
	mp_structure_destroy(s_low);
}

ZTEST(mp_structure_api, test_sanity)
{
	struct mp_structure *s = mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE,
						  MP_TYPE_INT, 48000, MP_STRUCTURE_END);

	zassert_is_null(mp_structure_get_value(s, MP_CAPS_IMAGE_WIDTH),
			"non-existent field != NULL");
	zassert_true(mp_structure_remove_field(s, MP_CAPS_IMAGE_WIDTH) < 0,
		     "remove non-existent field did not fail");

	mp_structure_destroy(s);
}
