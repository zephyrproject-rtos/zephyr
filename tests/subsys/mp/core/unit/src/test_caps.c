/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp.h>

extern struct k_heap _system_heap;

struct caps_fixture {
	struct mp_caps *caps[3];
	struct mp_caps *caps_intersect;
	struct mp_caps *caps_fixate;
	struct sys_memory_stats stats_before;
	struct sys_memory_stats stats_after;
	struct mp_structure *structure;
	struct mp_value *value;
};

static void *caps_suite_setup(void)
{
	static struct caps_fixture fixture;

	return &fixture;
}

static void caps_before(void *f)
{
	struct caps_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));
	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->stats_before);
}

static void caps_after(void *f)
{
	struct caps_fixture *fix = f;

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->stats_after);
	zassert_equal(fix->stats_before.allocated_bytes, fix->stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu",
		      fix->stats_before.allocated_bytes, fix->stats_after.allocated_bytes);
}

ZTEST_SUITE(caps, NULL, caps_suite_setup, caps_before, caps_after, NULL);

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

ZTEST_F(caps, test_caps_intersection_primitive)
{
	fixture->caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_BOOL, MP_TYPE_BOOLEAN, true,
				       TEST_INT, MP_TYPE_INT, -123, TEST_UINT, MP_TYPE_UINT, 123,
				       TEST_STRING, MP_TYPE_STRING, "xRGB", TEST_FRACTION,
				       MP_TYPE_INT_FRACTION, 30, 1, MP_CAPS_END);
	fixture->caps[2] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_BOOL, MP_TYPE_BOOLEAN, true,
				       TEST_INT, MP_TYPE_INT, -123, TEST_UINT, MP_TYPE_UINT, 123,
				       TEST_STRING, MP_TYPE_STRING, "xRGB", TEST_FRACTION,
				       MP_TYPE_INT_FRACTION, 30, 1, MP_CAPS_END);
	fixture->caps_intersect = mp_caps_intersect(fixture->caps[1], fixture->caps[1]);
	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);

	fixture->value = mp_structure_get_value(fixture->structure, TEST_BOOL);
	validate_boolean_value(fixture->value, true);

	fixture->value = mp_structure_get_value(fixture->structure, TEST_INT);
	validate_int_value(fixture->value, -123);

	fixture->value = mp_structure_get_value(fixture->structure, TEST_UINT);
	validate_uint_value(fixture->value, 123);

	fixture->value = mp_structure_get_value(fixture->structure, TEST_STRING);
	validate_string_value(fixture->value, "xRGB");

	fixture->value = mp_structure_get_value(fixture->structure, TEST_FRACTION);
	validate_int_fraction_value(fixture->value, 30, 1);

	mp_caps_unref(fixture->caps[1]);
	mp_caps_unref(fixture->caps[2]);
	mp_caps_unref(fixture->caps_intersect);
}

ZTEST_F(caps, test_caps_int_with_int_range)
{
	struct {
		int value;
		int expected;
	} test_cases[] = {
		{INT_MIN, INT_MIN},
		{INT_MAX, INT_MAX},
		{(INT_MIN + INT_MAX) / 2, (INT_MIN + INT_MAX) / 2},
	};

	fixture->caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_INT, MP_TYPE_INT_RANGE,
				       INT_MIN, INT_MAX, 1, MP_CAPS_END);
	zassert_not_null(fixture->caps[0], "caps[0] alloc failed");

	for (int i = 0; i < ARRAY_SIZE(test_cases); i++) {
		fixture->caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_INT, MP_TYPE_INT,
					       test_cases[i].value, MP_CAPS_END);
		zassert_not_null(fixture->caps[1], "caps[1] alloc failed");

		fixture->caps_intersect = mp_caps_intersect(fixture->caps[0], fixture->caps[1]);
		zassert_not_null(fixture->caps_intersect, "intersection returned NULL");

		fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
		fixture->value = mp_structure_get_value(fixture->structure, TEST_RANGE_INT);
		validate_int_value(fixture->value, test_cases[i].expected);

		mp_caps_unref(fixture->caps[1]);
		mp_caps_unref(fixture->caps_intersect);
	}

	mp_caps_unref(fixture->caps[0]);
}

ZTEST_F(caps, test_caps_uint_with_uint_range)
{
	struct {
		unsigned int expected;
		bool should_succeed;
		const char *description;
	} test_cases[] = {
		{0, true, "Zero value"},
		{UINT32_MAX, true, "Maximum value"},
		{UINT32_MAX / 2, true, "Mid-range value"},
	};

	fixture->caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_UINT, MP_TYPE_UINT_RANGE, 0,
				       UINT32_MAX, 1, MP_CAPS_END);
	zassert_not_null(fixture->caps[0], "caps[0] alloc failed");

	for (int i = 0; i < ARRAY_SIZE(test_cases); i++) {
		fixture->caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_RANGE_UINT, MP_TYPE_UINT,
					       test_cases[i].expected, MP_CAPS_END);
		zassert_not_null(fixture->caps[1], "caps[1] alloc failed for: %s",
				 test_cases[i].description);

		fixture->caps_intersect = mp_caps_intersect(fixture->caps[0], fixture->caps[1]);
		zassert_not_null(fixture->caps_intersect, "intersection returned NULL for: %s",
				 test_cases[i].description);

		fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
		fixture->value = mp_structure_get_value(fixture->structure, TEST_RANGE_UINT);
		validate_uint_value(fixture->value, test_cases[i].expected);

		mp_caps_unref(fixture->caps_intersect);
		mp_caps_unref(fixture->caps[1]);
	}

	mp_caps_unref(fixture->caps[0]);
}

ZTEST_F(caps, test_caps_int_fraction_range_intersection)
{
	fixture->caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_INT_FRACTION_RANGE,
				       MP_TYPE_INT_FRACTION, 1, INT_MIN, MP_CAPS_END);
	fixture->caps[1] =
		mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_INT_FRACTION_RANGE, MP_TYPE_INT_FRACTION_RANGE,
			    1, INT_MIN, INT_MAX, 1, 1, 1, MP_CAPS_END);

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[1], fixture->caps[1]);
	zassert_not_null(fixture->caps_intersect, "intersection returned NULL");
	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	fixture->value = mp_structure_get_value(fixture->structure, TEST_INT_FRACTION_RANGE);
	validate_fraction_int_range(fixture->value, 1, INT_MIN, INT_MAX, 1, 1, 1);
	mp_caps_unref(fixture->caps_intersect);

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[0], fixture->caps[1]);
	zassert_not_null(fixture->caps_intersect, "intersection returned NULL");
	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	fixture->value = mp_structure_get_value(fixture->structure, TEST_INT_FRACTION_RANGE);
	validate_int_fraction_value(fixture->value, 1, INT_MIN);

	mp_caps_unref(fixture->caps_intersect);
	mp_caps_unref(fixture->caps[0]);
	mp_caps_unref(fixture->caps[1]);
}

ZTEST_F(caps, test_caps_uint_fraction_range_intersection)
{
	fixture->caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_UINT_FRACTION_RANGE,
				       MP_TYPE_UINT_FRACTION, 1, UINT32_MAX, MP_CAPS_END);
	fixture->caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_UINT_FRACTION_RANGE,
				       MP_TYPE_UINT_FRACTION_RANGE, 1, UINT32_MAX, UINT32_MAX, 1, 1,
				       1, MP_CAPS_END);

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[1], fixture->caps[1]);
	zassert_not_null(fixture->caps_intersect, "intersection returned NULL");
	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	fixture->value = mp_structure_get_value(fixture->structure, TEST_UINT_FRACTION_RANGE);
	validate_uint_fraction_range(fixture->value, 1, UINT32_MAX, UINT32_MAX, 1, 1, 1);
	mp_caps_unref(fixture->caps_intersect);

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[0], fixture->caps[1]);
	zassert_not_null(fixture->caps_intersect, "intersection returned NULL");
	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	fixture->value = mp_structure_get_value(fixture->structure, TEST_UINT_FRACTION_RANGE);
	validate_uint_fraction_value(fixture->value, 1, UINT32_MAX);

	mp_caps_unref(fixture->caps_intersect);
	mp_caps_unref(fixture->caps[0]);
	mp_caps_unref(fixture->caps[1]);
}

ZTEST_F(caps, test_caps_int_fraction_range)
{
	fixture->caps[0] = mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_FRACTION, MP_TYPE_INT_FRACTION, 1,
				       INT32_MIN, MP_CAPS_END);
	fixture->caps[1] =
		mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_FRACTION, MP_TYPE_INT_FRACTION_RANGE, 1,
			    INT32_MIN, INT32_MAX, 1, 1, 1, MP_CAPS_END);
	fixture->caps[2] =
		mp_caps_new(MP_MEDIA_AUDIO_PCM, TEST_FRACTION, MP_TYPE_INT_FRACTION_RANGE, 1,
			    INT32_MIN, INT32_MAX, 1, 1, 1, MP_CAPS_END);

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[0], fixture->caps[1]);
	zassert_not_null(fixture->caps_intersect, "intersection returned NULL");

	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	fixture->value = mp_structure_get_value(fixture->structure, TEST_FRACTION);
	validate_int_fraction_value(fixture->value, 1, INT32_MIN);
	mp_caps_unref(fixture->caps_intersect);

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[1], fixture->caps[2]);
	zassert_not_null(fixture->caps_intersect, "intersection returned NULL");

	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	fixture->value = mp_structure_get_value(fixture->structure, TEST_FRACTION);
	validate_fraction_int_range(fixture->value, 1, INT_MIN, INT_MAX, 1, 1, 1);

	mp_caps_unref(fixture->caps[0]);
	mp_caps_unref(fixture->caps[1]);
	mp_caps_unref(fixture->caps[2]);
	mp_caps_unref(fixture->caps_intersect);
}

ZTEST_F(caps, test_caps_intersection_list)
{
	fixture->caps[0] = mp_caps_new(
		MP_MEDIA_AUDIO_PCM, TEST_LIST, MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15),
			     mp_value_new(MP_TYPE_UINT, 30),
			     mp_value_new(MP_TYPE_INT_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL), NULL),
		MP_CAPS_END);
	fixture->caps[1] = mp_caps_new(
		MP_MEDIA_AUDIO_PCM, TEST_LIST, MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_UINT, 30),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_INT_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_INT, 15), NULL),
		MP_CAPS_END);

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[0], fixture->caps[1]);
	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	struct mp_value *list = mp_structure_get_value(fixture->structure, TEST_LIST);

	validate_list_value_type_and_size(list, 7);

	struct mp_value *list_val = mp_value_list_get(list, 0);

	validate_int_value(list_val, 15);

	list_val = mp_value_list_get(list, 1);
	validate_uint_value(list_val, 30);

	list_val = mp_value_list_get(list, 2);
	validate_int_fraction_value(list_val, 15, 1);

	mp_caps_unref(fixture->caps[0]);
	mp_caps_unref(fixture->caps[1]);
	mp_caps_unref(fixture->caps_intersect);
}

ZTEST_F(caps, test_caps_video_sample)
{
	struct mp_value *frmrates1 = mp_value_new(MP_TYPE_LIST, NULL);

	for (int i = 15; i <= 60; i += 15) {
		zassert_ok(
			mp_value_list_append(frmrates1, mp_value_new(MP_TYPE_INT_FRACTION, i, 1)),
			"mp_value_list_append failed");
	}

	fixture->caps[0] = mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_STRING, "xRGB",
				       MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE, 1280, 1280, 0,
				       MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE, 720, 720, 0,
				       MP_CAPS_FRAME_RATE, MP_TYPE_LIST, frmrates1, MP_CAPS_END);
	zassert_not_null(fixture->caps[0], "caps[0] alloc failed");

	fixture->caps[1] =
		mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_LIST,
			    mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_STRING, "RGB565"),
					 mp_value_new(MP_TYPE_STRING, "xRGB"),
					 mp_value_new(MP_TYPE_STRING, "YUV"), NULL),
			    MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT_RANGE, 1280, 1280, 0,
			    MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT_RANGE, 720, 720, 0, MP_CAPS_END);
	zassert_not_null(fixture->caps[1], "caps[1] alloc failed");

	fixture->caps_intersect = mp_caps_intersect(fixture->caps[0], fixture->caps[1]);
	zassert_not_null(fixture->caps_intersect, "intersection returned NULL");
	zassert_false(mp_caps_is_any(fixture->caps_intersect), "caps is any");
	zassert_false(mp_caps_is_empty(fixture->caps_intersect), "caps is empty");

	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_PIXEL_FORMAT);

	validate_list_value_type_and_size(fixture->value, 1);
	zassert_str_equal(mp_value_get_string(mp_value_list_get(fixture->value, 0)), "xRGB");

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_IMAGE_WIDTH);
	validate_uint_range_value(fixture->value, 1280, 1280, 0);

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_IMAGE_HEIGHT);
	validate_uint_range_value(fixture->value, 720, 720, 0);

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_FRAME_RATE);
	validate_list_value_type_and_size(fixture->value, 4);

	for (int i = 15, j = 0; i <= 60; i += 15, j++) {
		struct mp_value *frac = mp_value_list_get(fixture->value, j);

		validate_int_fraction_value(frac, i, 1);
	}

	mp_caps_unref(fixture->caps[0]);
	mp_caps_unref(fixture->caps[1]);

	fixture->caps_fixate = mp_caps_fixate(fixture->caps_intersect);
	mp_caps_unref(fixture->caps_intersect);

	zassert_not_null(fixture->caps_fixate, "caps_fixate returned NULL");
	fixture->structure = mp_caps_get_structure(fixture->caps_fixate, 0);

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_PIXEL_FORMAT);
	validate_string_value(fixture->value, "xRGB");

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_IMAGE_WIDTH);
	validate_uint_value(fixture->value, 1280);

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_IMAGE_HEIGHT);
	validate_uint_value(fixture->value, 720);

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_FRAME_RATE);
	validate_int_fraction_value(fixture->value, 15, 1);

	mp_caps_unref(fixture->caps_fixate);
}

ZTEST_F(caps, test_caps_error_paths)
{
	struct mp_caps caps_stack;
	struct mp_caps *ptr = NULL;

	zassert_equal(mp_caps_init(NULL, 0), -EINVAL, "mp_caps_init(NULL) != -EINVAL");
	zassert_ok(mp_caps_init(&caps_stack, 0), "mp_caps_init failed");

	zassert_equal(mp_caps_replace(NULL, fixture->caps[0]), -EINVAL,
		      "mp_caps_replace(NULL target) != -EINVAL");
	zassert_equal(mp_caps_replace(&ptr, NULL), -EINVAL, "mp_caps_replace(NULL new) != -EINVAL");
}
