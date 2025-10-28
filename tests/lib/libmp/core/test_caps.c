/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include "mp.h"

ZTEST_SUITE(caps, NULL, NULL, NULL, NULL, NULL);

extern struct k_heap _system_heap;

ZTEST(caps, test_caps_intersection_primitive)
{
	struct sys_memory_stats stats_before, stats_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	/*case 1: test between primitives*/
	MpCaps *caps1 =
		mp_caps_new("test/x-primitive", "test-bool", MP_TYPE_BOOLEAN, true, "test-int",
			    MP_TYPE_INT, 123, "test-uint", MP_TYPE_UINT, 123, "test-string",
			    MP_TYPE_STRING, "xRGB", "test-fraction", MP_TYPE_FRACTION, 30, 1, NULL);
	MpCaps *caps2 =
		mp_caps_new("test/x-primitive", "test-bool", MP_TYPE_BOOLEAN, true, "test-int",
			    MP_TYPE_INT, 123, "test-uint", MP_TYPE_UINT, 123, "test-string",
			    MP_TYPE_STRING, "xRGB", "test-fraction", MP_TYPE_FRACTION, 30, 1, NULL);
	MpCaps *caps_intersect = mp_caps_intersect(caps1, caps2);

	MpStructure *structure = mp_caps_get_structure(caps_intersect, 0);
	MpValue *value = mp_structure_get_value(structure, "test-bool");
	zassert_not_null(value);
	zassert_equal(mp_value_get_boolean(value), true);

	value = mp_structure_get_value(structure, "test-int");
	zassert_not_null(value);
	zassert_equal(mp_value_get_int(value), 123);

	value = mp_structure_get_value(structure, "test-uint");
	zassert_not_null(value);
	zassert_equal(mp_value_get_int(value), 123);

	value = mp_structure_get_value(structure, "test-string");
	zassert_not_null(value);
	zassert_str_equal(mp_value_get_string(value), "xRGB");

	value = mp_structure_get_value(structure, "test-fraction");
	zassert_not_null(value);
	zassert_equal(mp_value_get_fraction_numerator(value), 30);
	zassert_equal(mp_value_get_fraction_denominator(value), 1);

	mp_caps_unref(caps1);
	mp_caps_unref(caps2);
	mp_caps_unref(caps_intersect);

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}

ZTEST(caps, test_caps_intersection_range)
{
	struct sys_memory_stats stats_before, stats_after;

	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_before);

	MpCaps *caps1 = mp_caps_new("test/x-range", "test-range", MP_TYPE_INT_RANGE, 1280, 1920, 1,
				    "test-range-int", MP_TYPE_INT, 1500, "test-fraction-range",
				    MP_TYPE_FRACTION_RANGE, 15, 1, 60, 1, 1, 1,
				    "test-fraction-range-fraction", MP_TYPE_FRACTION, 30, 1, NULL);

	MpCaps *caps2 = mp_caps_new("test/x-range", "test-range", MP_TYPE_INT_RANGE, 1500, 2000, 1,
				    "test-range-int", MP_TYPE_INT_RANGE, 1400, 1600, 1,
				    "test-fraction-range", MP_TYPE_FRACTION_RANGE, 30, 1, 90, 1, 1,
				    1, "test-fraction-range-fraction", MP_TYPE_FRACTION_RANGE, 20,
				    1, 40, 1, 1, 1, NULL);

	MpCaps *caps_intersect = mp_caps_intersect(caps1, caps2);
	MpStructure *structure = mp_caps_get_structure(caps_intersect, 0);
	MpValue *value = mp_structure_get_value(structure, "test-range");

	mp_caps_print(caps1);
	mp_caps_print(caps2);
	mp_caps_print(caps_intersect);

	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_INT_RANGE);
	zassert_equal(mp_value_get_int_range_min(value), 1500);
	zassert_equal(mp_value_get_int_range_max(value), 1920);
	zassert_equal(mp_value_get_int_range_step(value), 1);

	value = mp_structure_get_value(structure, "test-range-int");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_INT);
	zassert_equal(mp_value_get_int(value), 1500);

	value = mp_structure_get_value(structure, "test-fraction-range");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_FRACTION_RANGE);

	MpValue *frac = mp_value_get_fraction_range_min(value);
	zassert_equal(mp_value_get_fraction_numerator(frac), 30);
	zassert_equal(mp_value_get_fraction_denominator(frac), 1);
	frac = mp_value_get_fraction_range_max(value);
	zassert_equal(mp_value_get_fraction_numerator(frac), 60);
	zassert_equal(mp_value_get_fraction_denominator(frac), 1);
	frac = mp_value_get_fraction_range_step(value);
	zassert_equal(mp_value_get_fraction_numerator(frac), 1);
	zassert_equal(mp_value_get_fraction_denominator(frac), 1);
	frac = mp_structure_get_value(structure, "test-fraction-range-fraction");
	zassert_not_null(frac);
	zassert_equal(frac->type, MP_TYPE_FRACTION);
	zassert_equal(mp_value_get_fraction_numerator(frac), 30);
	zassert_equal(mp_value_get_fraction_denominator(frac), 1);

	mp_caps_unref(caps1);
	mp_caps_unref(caps2);
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

	MpCaps *caps1 = mp_caps_new(
		"test/x-list", "list", MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15),
			     mp_value_new(MP_TYPE_UINT, 30), mp_value_new(MP_TYPE_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL), NULL),
		NULL);
	MpCaps *caps2 = mp_caps_new(
		"test/x-list", "list", MP_TYPE_LIST,
		mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_STRING, "RGB"),
			     mp_value_new(MP_TYPE_UINT, 30),
			     mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_INT, 15), NULL),
			     mp_value_new(MP_TYPE_INT_RANGE, 1, 100, 1),
			     mp_value_new(MP_TYPE_FRACTION, 15, 1),
			     mp_value_new(MP_TYPE_FRACTION_RANGE, 100, 1, 60, 1, 1, 1),
			     mp_value_new(MP_TYPE_INT, 15), NULL),
		NULL);
	MpCaps *caps_intersect = mp_caps_intersect(caps1, caps2);

	mp_caps_print(caps1);
	mp_caps_print(caps2);
	mp_caps_print(caps_intersect);

	MpStructure *structure = mp_caps_get_structure(caps_intersect, 0);
	MpValue *list = mp_structure_get_value(structure, "list");

	zassert_equal(mp_value_list_get_size(list), 7, "list size: %d",
		      mp_value_list_get_size(list));

	MpValue *value = mp_value_list_get(list, 0);
	zassert_not_null(value);
	zassert_equal(mp_value_get_int(value), 15);
	value = mp_value_list_get(list, 1);
	zassert_not_null(value);
	zassert_equal(mp_value_get_uint(value), 30);
	value = mp_value_list_get(list, 2);
	zassert_not_null(value);
	zassert_equal(mp_value_get_fraction_numerator(value), 15);
	zassert_equal(mp_value_get_fraction_denominator(value), 1);

	mp_caps_unref(caps1);
	mp_caps_unref(caps2);
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

	MpValue *frmrates1 = mp_value_new(MP_TYPE_LIST, NULL);

	/** Generate different framerates */
	for (int i = 15; i <= 60; i += 15) {
		mp_value_list_append(frmrates1, mp_value_new(MP_TYPE_FRACTION, i, 1, NULL));
	}

	/* caps1: video/x-raw, format(string)=xRGB, width(int_range)=[1280, 1280 ,0],
	 * height(int_range)=[720, 720, 0], framerate={15/1,30/1,45/1,60/1} */

	MpCaps *caps1 = mp_caps_new("video/x-raw", "format", MP_TYPE_STRING, "xRGB", "width",
				    MP_TYPE_INT_RANGE, 1280, 1280, 0, "height", MP_TYPE_INT_RANGE,
				    720, 720, 0, "frmrate", MP_TYPE_LIST, frmrates1, NULL);
	zassert_not_null(caps1, "caps1 allocation failed");

	/* caps2: video/x-raw, format(string)={RGB565, xRGB, YUV}, width(int_range)=[1280, 1280 ,0],
	 * height(int_range)=[720, 720, 0]
	 */
	MpCaps *caps2 =
		mp_caps_new("video/x-raw", "format", MP_TYPE_LIST,
			    mp_value_new(MP_TYPE_LIST, mp_value_new(MP_TYPE_STRING, "RGB565", NULL),
					 mp_value_new(MP_TYPE_STRING, "xRGB", NULL),
					 mp_value_new(MP_TYPE_STRING, "YUV", NULL), NULL),
			    "width", MP_TYPE_INT_RANGE, 1280, 1280, 0, "height", MP_TYPE_INT_RANGE,
			    720, 720, 0, NULL);
	zassert_not_null(caps2, "caps2 allocation failed");

	MpCaps *caps_intersect = mp_caps_intersect(caps1, caps2);

	printk("\ncaps1:");
	mp_caps_print(caps1);

	printk("\ncaps2:");
	mp_caps_print(caps2);

	printk("caps_intersect:\n");
	mp_caps_print(caps_intersect);

	zassert_not_null(caps_intersect, "Intersection failed");
	zassert_false(mp_caps_is_any(caps_intersect), "caps is any");
	zassert_false(mp_caps_is_empty(caps_intersect), "caps is empty");

	/* Check intersection result */
	MpStructure *structure = mp_caps_get_structure(caps_intersect, 0);
	MpValue *value = mp_structure_get_value(structure, "format");
	zassert_equal(value->type, MP_TYPE_LIST);
	zassert_str_equal(mp_value_get_string(mp_value_list_get(value, 0)), "xRGB");

	value = mp_structure_get_value(structure, "width");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_INT_RANGE);
	zassert_equal(mp_value_get_int_range_max(value), 1280);
	zassert_equal(mp_value_get_int_range_min(value), 1280);
	zassert_equal(mp_value_get_int_range_step(value), 0);

	value = mp_structure_get_value(structure, "height");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_INT_RANGE);
	zassert_equal(mp_value_get_int_range_max(value), 720);
	zassert_equal(mp_value_get_int_range_min(value), 720);
	zassert_equal(mp_value_get_int_range_step(value), 0);

	value = mp_structure_get_value(structure, "frmrate");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_LIST);

	for (int i = 15, j = 0; i <= 60; i += 15, j++) {
		MpValue *frac = mp_value_list_get(value, j);
		zassert_not_null(frac);
		zassert_equal(frac->type, MP_TYPE_FRACTION);
		zassert_equal(mp_value_get_fraction_numerator(frac), i,
			      "mp_value_get_fraction_numerator(value) %d",
			      mp_value_get_fraction_numerator(frac));
		zassert_equal(mp_value_get_fraction_denominator(frac), 1);
	}
	mp_caps_unref(caps2);
	mp_caps_unref(caps1);

	/* check fixate */
	MpCaps *caps_fixate = mp_caps_fixate(caps_intersect);
	mp_caps_unref(caps_intersect);
	zassert_not_null(caps_fixate);
	structure = mp_caps_get_structure(caps_fixate, 0);
	value = mp_structure_get_value(structure, "format");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_STRING);
	zassert_str_equal(mp_value_get_string(value), "xRGB");

	value = mp_structure_get_value(structure, "width");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_INT);
	zassert_equal(mp_value_get_int(value), 1280);

	value = mp_structure_get_value(structure, "height");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_INT);
	zassert_equal(mp_value_get_int(value), 720);

	value = mp_structure_get_value(structure, "frmrate");
	zassert_not_null(value);
	zassert_equal(value->type, MP_TYPE_FRACTION);
	zassert_equal(mp_value_get_fraction_numerator(value), 15);
	zassert_equal(mp_value_get_fraction_denominator(value), 1);

	// Free all allocated memory
	mp_caps_unref(caps_fixate);
	sys_heap_runtime_stats_get(&_system_heap.heap, &stats_after);
	zassert_equal(stats_before.allocated_bytes, stats_after.allocated_bytes,
		      "Memory leak detected: before=%zu, after=%zu", stats_before.allocated_bytes,
		      stats_after.allocated_bytes);
}
