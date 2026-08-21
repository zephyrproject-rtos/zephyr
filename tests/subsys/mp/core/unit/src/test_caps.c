/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/mp.h>

#include "mp_test_helpers.h"

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

ZTEST_F(caps, test_caps_any_empty_intersections)
{
	struct mp_caps *caps_any1 = mp_caps_new_any();
	struct mp_caps *caps_any2 = mp_caps_new_any();
	struct mp_caps *caps_empty1 = mp_caps_new(MP_MEDIA_END);
	struct mp_caps *caps_empty2 = mp_caps_new(MP_MEDIA_END);

	zassert_not_null(caps_any1, "caps_any1 alloc failed");
	zassert_not_null(caps_any2, "caps_any2 alloc failed");
	zassert_not_null(caps_empty1, "caps_empty1 alloc failed");
	zassert_not_null(caps_empty2, "caps_empty2 alloc failed");

	zassert_true(mp_caps_is_any(caps_any1), "caps_any1 must be ANY");
	zassert_true(mp_caps_is_empty(caps_empty1), "caps_empty1 must be EMPTY");

	fixture->caps[0] =
		mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT, 7, MP_CAPS_END);
	zassert_not_null(fixture->caps[0], "explicit caps alloc failed");

	struct mp_caps *result = mp_caps_intersect(caps_any1, caps_any2);

	zassert_not_null(result, "ANY & ANY must not return NULL");
	zassert_true(mp_caps_is_any(result), "ANY & ANY must be ANY");
	mp_caps_unref(result);

	result = mp_caps_intersect(caps_any1, fixture->caps[0]);
	zassert_not_null(result, "ANY & explicit must not return NULL");
	zassert_false(mp_caps_is_any(result), "ANY & explicit must not be ANY");
	zassert_false(mp_caps_is_empty(result), "ANY & explicit must not be EMPTY");
	fixture->structure = mp_caps_get_structure(result, 0);
	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_SAMPLE_RATE);
	validate_int_value(fixture->value, 7);
	mp_caps_unref(result);

	result = mp_caps_intersect(caps_empty1, caps_empty2);
	zassert_is_null(result, "EMPTY & EMPTY must return NULL");

	result = mp_caps_intersect(caps_empty1, fixture->caps[0]);
	zassert_is_null(result, "EMPTY & explicit must return NULL");

	result = mp_caps_intersect(caps_empty1, caps_any1);
	zassert_is_null(result, "EMPTY & ANY must return NULL");

	result = mp_caps_intersect(fixture->caps[0], NULL);
	zassert_is_null(result, "intersect(caps, NULL) must return NULL");

	result = mp_caps_intersect(NULL, NULL);
	zassert_is_null(result, "intersect(NULL, NULL) must return NULL");

	mp_caps_unref(caps_any1);
	mp_caps_unref(caps_any2);
	mp_caps_unref(caps_empty1);
	mp_caps_unref(caps_empty2);
	mp_caps_unref(fixture->caps[0]);
	fixture->caps[0] = NULL;
}

ZTEST_F(caps, test_caps_multi_structure)
{
	struct mp_caps *caps_multi = mp_caps_new(MP_MEDIA_END);

	zassert_not_null(caps_multi, "multi-structure caps alloc failed");

	/* First audio structure: sample rate range 8000-16000 */
	struct mp_structure *audio_s1 =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT_RANGE, 8000,
				 16000, 8000, MP_CAPS_BITWIDTH, MP_TYPE_INT, 16, MP_STRUCTURE_END);

	zassert_not_null(audio_s1, "audio_s1 alloc failed");
	zassert_ok(mp_caps_append(caps_multi, audio_s1), "append audio_s1 failed");

	/* Second audio structure: sample rate range 32000-48000 */
	struct mp_structure *audio_s2 =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT_RANGE, 32000,
				 48000, 8000, MP_CAPS_BITWIDTH, MP_TYPE_INT, 24, MP_STRUCTURE_END);

	zassert_not_null(audio_s2, "audio_s2 alloc failed");
	zassert_ok(mp_caps_append(caps_multi, audio_s2), "append audio_s2 failed");

	/** Test fixate */
	fixture->caps_fixate = mp_caps_fixate(caps_multi);
	zassert_not_null(fixture->caps_fixate, "mp_caps_fixate returned NULL");

	fixture->structure = mp_caps_get_structure(fixture->caps_fixate, 0);
	zassert_not_null(fixture->structure, "fixated structure 0 is NULL");
	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_SAMPLE_RATE);
	validate_int_value(fixture->value, 8000);
	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_BITWIDTH);
	validate_int_value(fixture->value, 16);

	fixture->structure = mp_caps_get_structure(fixture->caps_fixate, 1);
	zassert_is_null(fixture->structure, "fixated structure 1 must be NULL");

	mp_caps_unref(fixture->caps_fixate);
	fixture->caps_fixate = NULL;

	/* caps[1]: fixed sample rate 44100 - only compatible with the second structure */
	fixture->caps[1] = mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT, 44100,
				       MP_CAPS_BITWIDTH, MP_TYPE_INT, 24, MP_CAPS_END);
	zassert_not_null(fixture->caps[1], "caps[1] alloc failed");

	fixture->caps_intersect = mp_caps_intersect(caps_multi, fixture->caps[1]);
	zassert_not_null(fixture->caps_intersect, "multi-structure intersection returned NULL");

	/* Result must contain exactly one structure */
	fixture->structure = mp_caps_get_structure(fixture->caps_intersect, 0);
	zassert_not_null(fixture->structure, "intersected structure at index 0 is NULL");
	zassert_equal(fixture->structure->media_type_id, MP_MEDIA_AUDIO_PCM,
		      "intersected structure is not audio");

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_SAMPLE_RATE);
	validate_int_value(fixture->value, 44100);

	fixture->value = mp_structure_get_value(fixture->structure, MP_CAPS_BITWIDTH);
	validate_int_value(fixture->value, 24);

	zassert_is_null(mp_caps_get_structure(fixture->caps_intersect, 1),
			"unexpected second structure in intersection result");

	mp_caps_unref(caps_multi);
	mp_caps_unref(fixture->caps[1]);
	mp_caps_unref(fixture->caps_intersect);
	fixture->caps[1] = NULL;
	fixture->caps_intersect = NULL;
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
