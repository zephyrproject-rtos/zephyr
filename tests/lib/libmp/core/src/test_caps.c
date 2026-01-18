/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "mp.h"

ZTEST_SUITE(caps, NULL, NULL, NULL, NULL, NULL);

extern struct k_heap _system_heap;

struct mp_caps *caps[3];
struct mp_caps *caps_intersect;
struct mp_caps *caps_fixate;
struct sys_memory_stats stats_before, stats_after;
struct mp_structure *structure;
struct mp_value *value;

#define validate_boolean_value(value, expected)                                                    \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_BOOLEAN);                                     \
		zassert_equal(mp_value_get_boolean(value), expected);                              \
	})

#define validate_int_value(value, expected)                                                        \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT);                                         \
		zassert_equal(mp_value_get_int(value), expected);                                  \
	})

#define validate_uint_value(value, expected)                                                       \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT);                                        \
		zassert_equal(mp_value_get_uint(value), expected);                                 \
	})

#define validate_string_value(value, expected)                                                     \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_STRING);                                      \
		zassert_str_equal(mp_value_get_string(value), expected);                           \
	})

#define validate_int_fraction_value(value, num, denom)                                             \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT_FRACTION);                                \
		zassert_equal(mp_value_get_fraction_numerator(value), num);                        \
		zassert_equal(mp_value_get_fraction_denominator(value), denom);                    \
	})

#define validate_uint_fraction_value(value, num, denom)                                            \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT_FRACTION);                               \
		zassert_equal(mp_value_get_fraction_numerator(value), num);                        \
		zassert_equal(mp_value_get_fraction_denominator(value), denom);                    \
	})

#define validate_int_range_value(value, min, max, step)                                            \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT_RANGE);                                   \
		zassert_equal(mp_value_get_int_range_min(value), min);                             \
		zassert_equal(mp_value_get_int_range_max(value), max);                             \
		zassert_equal(mp_value_get_int_range_step(value), step);                           \
	})

#define validate_uint_range_value(value, min, max, step)                                           \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT_RANGE);                                  \
		zassert_equal(mp_value_get_uint_range_min(value), min);                            \
		zassert_equal(mp_value_get_uint_range_max(value), max);                            \
		zassert_equal(mp_value_get_uint_range_step(value), step);                          \
	})

#define validate_fraction_int_range(value, min_num, min_denom, max_num, max_denom, step_num,       \
				    step_denom)                                                    \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_INT_FRACTION_RANGE);                          \
		const struct mp_value *frac = mp_value_get_fraction_range_min(value);              \
		zassert_equal(mp_value_get_fraction_numerator(frac), min_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), min_denom);                 \
		frac = mp_value_get_fraction_range_max(value);                                     \
		zassert_equal(mp_value_get_fraction_numerator(frac), max_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), max_denom);                 \
		frac = mp_value_get_fraction_range_step(value);                                    \
		zassert_equal(mp_value_get_fraction_numerator(frac), step_num);                    \
		zassert_equal(mp_value_get_fraction_denominator(frac), step_denom);                \
	})

#define validate_uint_fraction_range(value, min_num, min_denom, max_num, max_denom, step_num,      \
				     step_denom)                                                   \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_UINT_FRACTION_RANGE);                         \
		const struct mp_value *frac = mp_value_get_fraction_range_min(value);              \
		zassert_equal(mp_value_get_fraction_numerator(frac), min_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), min_denom);                 \
		frac = mp_value_get_fraction_range_max(value);                                     \
		zassert_equal(mp_value_get_fraction_numerator(frac), max_num);                     \
		zassert_equal(mp_value_get_fraction_denominator(frac), max_denom);                 \
		frac = mp_value_get_fraction_range_step(value);                                    \
		zassert_equal(mp_value_get_fraction_numerator(frac), step_num);                    \
		zassert_equal(mp_value_get_fraction_denominator(frac), step_denom);                \
	})

#define validate_list_value_type_and_size(value, expected_size)                                    \
	({                                                                                         \
		zassert_not_null(value);                                                           \
		zassert_equal((value)->type, MP_TYPE_LIST);                                        \
		zassert_equal(mp_value_list_get_size(value), expected_size);                       \
	})

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

ZTEST(caps, test_caps_intersection_primitive)
{
	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_BOOL, MP_TYPE_BOOLEAN, true, TEST_INT,
			      MP_TYPE_INT, -123, TEST_UINT, MP_TYPE_UINT, 123, TEST_STRING,
			      MP_TYPE_STRING, "xRGB", TEST_FRACTION, MP_TYPE_INT_FRACTION, 30, 1,
			      MP_CAPS_END);
	caps[2] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_BOOL, MP_TYPE_BOOLEAN, true, TEST_INT,
			      MP_TYPE_INT, -123, TEST_UINT, MP_TYPE_UINT, 123, TEST_STRING,
			      MP_TYPE_STRING, "xRGB", TEST_FRACTION, MP_TYPE_INT_FRACTION, 30, 1,
			      MP_CAPS_END);
	caps_intersect = mp_caps_intersect(caps[1], caps[1]);
	structure = mp_caps_get_structure(caps_intersect, 0);

	value = mp_structure_get_value(structure, TEST_BOOL);
	validate_boolean_value(value, true);

	value = mp_structure_get_value(structure, TEST_INT);
	validate_int_value(value, -123);

	value = mp_structure_get_value(structure, TEST_UINT);
	validate_uint_value(value, 123);

	value = mp_structure_get_value(structure, TEST_STRING);
	validate_string_value(value, "xRGB");

	value = mp_structure_get_value(structure, TEST_FRACTION);
	validate_int_fraction_value(value, 30, 1);

	mp_caps_unref(caps[1]);
	mp_caps_unref(caps[2]);
	mp_caps_unref(caps_intersect);

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

ZTEST(caps, test_caps_int_with_int_range)
{
	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	/* Prepare test cases: INT values and expected results */
	struct {
		int value;
		int expected;
	} test_cases[] = {
		{INT_MIN, INT_MIN},
		{INT_MAX, INT_MAX},
		{(INT_MIN + INT_MAX) / 2, (INT_MIN + INT_MAX) / 2},
	};

	/* Create caps for INT_RANGE */
	caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_INT, MP_TYPE_INT_RANGE, INT_MIN,
			      INT_MAX, 1, MP_CAPS_END);
	zassert_not_null(caps[0], "Failed to create INT_RANGE caps");

	/* Loop through test cases */
	for (int i = 0; i < ARRAY_SIZE(test_cases); i++) {
		caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_INT, MP_TYPE_INT,
				      test_cases[i].value, MP_CAPS_END);
		zassert_not_null(caps[1], "Failed to create INT caps");

		caps_intersect = mp_caps_intersect(caps[0], caps[1]);
		zassert_not_null(caps_intersect, "Intersection should not be null");

		structure = mp_caps_get_structure(caps_intersect, 0);
		value = mp_structure_get_value(structure, TEST_RANGE_INT);
		validate_int_value(value, test_cases[i].expected);

		mp_caps_unref(caps[1]);
		mp_caps_unref(caps_intersect);
	}

	/* Cleanup */
	mp_caps_unref(caps[0]);

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

/* Test UINT value intersecting with UINT_RANGE */
ZTEST(caps, test_caps_uint_with_uint_range)
{
	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	/* Prepare test cases: UINT values and expected results */
	struct {
		unsigned int expected;
		bool should_succeed;
		const char *description;
	} test_cases[] = {
		{0, true, "Zero value"},
		{UINT32_MAX, true, "Maximum value"},
		{UINT32_MAX / 2, true, "Mid-range value"},
	};

	caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_UINT, MP_TYPE_UINT_RANGE, 0,
			      UINT32_MAX, 1, MP_CAPS_END);
	zassert_not_null(caps[0], "Failed to create UINT_RANGE caps");

	/* Create caps for UINT_RANGE that will be reused */
	for (int i = 0; i < ARRAY_SIZE(test_cases); i++) {
		caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_UINT, MP_TYPE_UINT,
				      test_cases[i].expected, MP_CAPS_END);
		zassert_not_null(caps[1], "Failed to create UINT caps for case: %s",
				 test_cases[i].description);

		caps_intersect = mp_caps_intersect(caps[0], caps[1]);
		zassert_not_null(caps_intersect, "Intersection should succeed for case: %s",
				 test_cases[i].description);

		structure = mp_caps_get_structure(caps_intersect, 0);
		value = mp_structure_get_value(structure, TEST_RANGE_UINT);
		validate_uint_value(value, test_cases[i].expected);

		mp_caps_unref(caps_intersect);
		mp_caps_unref(caps[1]);
	}

	mp_caps_unref(caps[0]);
	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

/* Test INT_FRACTION_RANGE intersecting with INT_FRACTION_RANGE */
ZTEST(caps, test_caps_int_fraction_range_intersection)
{
	struct sys_memory_stats stats_before, stats_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_INT_FRACTION_RANGE, MP_TYPE_INT_FRACTION, 1,
			      INT_MIN, MP_CAPS_END);
	caps[1] =
		mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_INT_FRACTION_RANGE, MP_TYPE_INT_FRACTION_RANGE,
			    1, INT_MIN, INT_MAX, 1, 1, 1, MP_CAPS_END);

	caps_intersect = mp_caps_intersect(caps[1], caps[1]);
	zassert_not_null(caps_intersect, "Intersection should not be null");
	structure = mp_caps_get_structure(caps_intersect, 0);
	value = mp_structure_get_value(structure, TEST_INT_FRACTION_RANGE);
	validate_fraction_int_range(value, 1, INT_MIN, INT_MAX, 1, 1, 1);
	mp_caps_unref(caps_intersect);

	caps_intersect = mp_caps_intersect(caps[0], caps[1]);
	zassert_not_null(caps_intersect, "Intersection should not be null");
	structure = mp_caps_get_structure(caps_intersect, 0);
	value = mp_structure_get_value(structure, TEST_INT_FRACTION_RANGE);
	validate_int_fraction_value(value, 1, INT_MIN);

	mp_caps_unref(caps_intersect);
	mp_caps_unref(caps[0]);
	mp_caps_unref(caps[1]);

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

/* Test UINT_FRACTION_RANGE intersecting with UINT_FRACTION_RANGE */
ZTEST(caps, test_caps_uint_fraction_range_intersection)
{
	struct sys_memory_stats stats_before, stats_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_UINT_FRACTION_RANGE, MP_TYPE_UINT_FRACTION,
			      1, UINT32_MAX, MP_CAPS_END);
	caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_UINT_FRACTION_RANGE,
			      MP_TYPE_UINT_FRACTION_RANGE, 1, UINT32_MAX, UINT32_MAX, 1, 1, 1,
			      MP_CAPS_END);

	caps_intersect = mp_caps_intersect(caps[1], caps[1]);
	zassert_not_null(caps_intersect, "Intersection should not be null");
	structure = mp_caps_get_structure(caps_intersect, 0);
	value = mp_structure_get_value(structure, TEST_UINT_FRACTION_RANGE);
	validate_uint_fraction_range(value, 1, UINT32_MAX, UINT32_MAX, 1, 1, 1);
	mp_caps_unref(caps_intersect);

	caps_intersect = mp_caps_intersect(caps[0], caps[1]);
	zassert_not_null(caps_intersect, "Intersection should not be null");
	structure = mp_caps_get_structure(caps_intersect, 0);
	value = mp_structure_get_value(structure, TEST_UINT_FRACTION_RANGE);
	validate_uint_fraction_value(value, 1, UINT32_MAX);

	mp_caps_unref(caps_intersect);
	mp_caps_unref(caps[0]);
	mp_caps_unref(caps[1]);

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

/* Test INT_FRACTION intersecting with INT_FRACTION_RANGE */
ZTEST(caps, test_caps_int_fraction_range)
{
	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_FRACTION, MP_TYPE_INT_FRACTION, 1, INT32_MIN,
			      MP_CAPS_END);
	caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_FRACTION, MP_TYPE_INT_FRACTION_RANGE, 1,
			      INT32_MIN, INT32_MAX, 1, 1, 1, MP_CAPS_END);
	caps[2] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_FRACTION, MP_TYPE_INT_FRACTION_RANGE, 1,
			      INT32_MIN, INT32_MAX, 1, 1, 1, MP_CAPS_END);
	caps_intersect = mp_caps_intersect(caps[0], caps[1]);
	zassert_not_null(caps_intersect, "Intersection should not be null");

	structure = mp_caps_get_structure(caps_intersect, 0);
	value = mp_structure_get_value(structure, TEST_FRACTION);
	validate_int_fraction_value(value, 1, INT32_MIN);
	mp_caps_unref(caps_intersect);

	caps_intersect = mp_caps_intersect(caps[1], caps[2]);
	zassert_not_null(caps_intersect, "Intersection should not be null");

	structure = mp_caps_get_structure(caps_intersect, 0);
	value = mp_structure_get_value(structure, TEST_FRACTION);
	validate_fraction_int_range(value, 1, INT_MIN, INT_MAX, 1, 1, 1);

	mp_caps_unref(caps[0]);
	mp_caps_unref(caps[1]);
	mp_caps_unref(caps[2]);
	mp_caps_unref(caps_intersect);

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

ZTEST(caps, test_caps_intersection_list)
{
	struct sys_memory_stats stats_before, stats_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	caps[0] = mp_caps_new(
		MP_MEDIA_AUDIO_PCM, TEST_LIST, MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15),
			     mp_value_new(MP_TYPE_UINT, 30),
			     mp_value_new(MP_TYPE_INT_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL), NULL),
		MP_CAPS_END);
	caps[1] = mp_caps_new(
		MP_MEDIA_AUDIO_PCM, TEST_LIST, MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_UINT, 30),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_INT, 15), NULL),
		MP_CAPS_END);
	caps_intersect = mp_caps_intersect(caps[0], caps[1]);

	mp_caps_print(caps[0]);
	mp_caps_print(caps[1]);
	mp_caps_print(caps_intersect);

	struct mp_structure *structure = mp_caps_get_structure(caps_intersect, 0);
	struct mp_value *list = mp_structure_get_value(structure, TEST_LIST);

	validate_list_value_type_and_size(list, 7);

	struct mp_value *value = mp_value_list_get(list, 0);

	validate_int_value(value, 15);

	value = mp_value_list_get(list, 1);
	validate_uint_value(value, 30);

	value = mp_value_list_get(list, 2);
	validate_int_fraction_value(value, 15, 1);

	mp_caps_unref(caps[0]);
	mp_caps_unref(caps[1]);
	mp_caps_unref(caps_intersect);

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

ZTEST(caps, test_caps_video_sample)
{
	struct sys_memory_stats stats_before, stats_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	struct mp_value *frmrates1 = mp_value_new(MP_TYPE_LIST, NULL);

	/* Generate different framerates */
	for (int i = 15; i <= 60; i += 15) {
		mp_value_list_append(frmrates1, mp_value_new(MP_TYPE_INT_FRACTION, i, 1));
	}

	/* caps[0]: video/x-raw, format(string)=xRGB, width(int_range)=[1280, 1280 ,0],
	 * height(int_range)=[720, 720, 0], framerate={15/1,30/1,45/1,60/1}
	 */
	caps[0] = mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_STRING, "xRGB",
			      MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE, 1280, 1280, 0,
			      MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE, 720, 720, 0,
			      MP_CAPS_FRAME_RATE, MP_TYPE_LIST, frmrates1, MP_CAPS_END);

	zassert_not_null(caps[0], "caps[0] allocation failed");

	/* caps[1]: video/x-raw, format(string)={RGB565, xRGB, YUV}, width(int_range)=[1280, 1280
	 * ,0], height(int_range)=[720, 720, 0]
	 */
	caps[1] = mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_LIST,
			      mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_STRING, "RGB565"),
					   mp_value_new(MP_TYPE_STRING, "xRGB"),
					   mp_value_new(MP_TYPE_STRING, "YUV"), NULL),
			      MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE, 1280, 1280, 0,
			      MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE, 720, 720, 0, MP_CAPS_END);

	zassert_not_null(caps[1], "caps[1] allocation failed");

	caps_intersect = mp_caps_intersect(caps[0], caps[1]);

	printk("\ncaps[0]:");
	mp_caps_print(caps[0]);

	printk("\ncaps[1]:");
	mp_caps_print(caps[1]);

	printk("caps_intersect:\n");
	mp_caps_print(caps_intersect);

	zassert_not_null(caps_intersect, "Intersection failed");
	zassert_false(mp_caps_is_any(caps_intersect), "caps is any");
	zassert_false(mp_caps_is_empty(caps_intersect), "caps is empty");

	/* Check intersection result */
	structure = mp_caps_get_structure(caps_intersect, 0);
	value = mp_structure_get_value(structure, MP_CAPS_PIXEL_FORMAT);

	validate_list_value_type_and_size(value, 1);
	zassert_str_equal(mp_value_get_string(mp_value_list_get(value, 0)), "xRGB");

	value = mp_structure_get_value(structure, MP_CAPS_IMAGE_WIDTH);
	validate_uint_range_value(value, 1280, 1280, 0);

	value = mp_structure_get_value(structure, MP_CAPS_IMAGE_HEIGHT);
	validate_uint_range_value(value, 720, 720, 0);

	value = mp_structure_get_value(structure, MP_CAPS_FRAME_RATE);
	validate_list_value_type_and_size(value, 4);

	for (int i = 15, j = 0; i <= 60; i += 15, j++) {
		struct mp_value *frac = mp_value_list_get(value, j);

		validate_int_fraction_value(frac, i, 1);
	}
	mp_caps_unref(caps[0]);
	mp_caps_unref(caps[1]);

	/* check fixate */
	caps_fixate = mp_caps_fixate(caps_intersect);

	mp_caps_unref(caps_intersect);
	zassert_not_null(caps_fixate);
	structure = mp_caps_get_structure(caps_fixate, 0);

	value = mp_structure_get_value(structure, MP_CAPS_PIXEL_FORMAT);
	validate_string_value(value, "xRGB");

	value = mp_structure_get_value(structure, MP_CAPS_IMAGE_WIDTH);
	validate_uint_value(value, 1280);

	value = mp_structure_get_value(structure, MP_CAPS_IMAGE_HEIGHT);
	validate_uint_value(value, 720);

	value = mp_structure_get_value(structure, MP_CAPS_FRAME_RATE);
	validate_int_fraction_value(value, 15, 1);

	/* Free all allocated memory */
	mp_caps_unref(caps_fixate);
	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}
