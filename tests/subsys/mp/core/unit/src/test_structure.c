/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

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

ZTEST(mp_structure_api, test_new)
{
	struct mp_structure *s =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT, 48000,
				 MP_CAPS_BITWIDTH, MP_TYPE_INT, 16, MP_STRUCTURE_END);

	zassert_not_null(s, "mp_structure_new returned NULL");
	zassert_equal(s->media_type_id, MP_MEDIA_AUDIO_PCM, "media_type_id mismatch");

	struct mp_value *rate = mp_structure_get_value(s, MP_CAPS_SAMPLE_RATE);

	zassert_not_null(rate, "SAMPLE_RATE field not found");
	zassert_equal(mp_value_get_int(rate), 48000, "sample rate != 48000");

	struct mp_value *bw = mp_structure_get_value(s, MP_CAPS_BITWIDTH);

	zassert_not_null(bw, "BITWIDTH field not found");
	zassert_equal(mp_value_get_int(bw), 16, "bitwidth != 16");

	zassert_ok(mp_structure_remove_field(s, MP_CAPS_SAMPLE_RATE), "remove_field failed");
	zassert_is_null(mp_structure_get_value(s, MP_CAPS_SAMPLE_RATE),
			"removed field still found");
	zassert_not_null(mp_structure_get_value(s, MP_CAPS_BITWIDTH), "non-removed field missing");

	mp_structure_clear(s);
	zassert_is_null(mp_structure_get_value(s, MP_CAPS_BITWIDTH), "field found after clear");
	mp_structure_destroy(s);

	struct mp_structure *sv = mp_structure_new(MP_MEDIA_VIDEO, MP_STRUCTURE_END);

	zassert_not_null(sv, "mp_structure_new(no fields) returned NULL");
	zassert_equal(sv->media_type_id, MP_MEDIA_VIDEO, "media_type_id != VIDEO");
	mp_structure_destroy(sv);

	struct mp_structure *sr =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT_RANGE, 8000,
				 48000, 8000, MP_STRUCTURE_END);

	zassert_not_null(sr);
	struct mp_value *val = mp_structure_get_value(sr, MP_CAPS_SAMPLE_RATE);

	zassert_not_null(val);
	zassert_equal(val->type, MP_TYPE_INT_RANGE, "type != INT_RANGE");
	zassert_equal(mp_value_get_int_range_min(val), 8000, "min != 8000");
	zassert_equal(mp_value_get_int_range_max(val), 48000, "max != 48000");
	mp_structure_destroy(sr);

	struct mp_structure si;

	zassert_ok(mp_structure_init(&si, MP_MEDIA_AUDIO_PCM), "mp_structure_init failed");
	zassert_equal(si.media_type_id, MP_MEDIA_AUDIO_PCM, "media_type_id mismatch");

	struct mp_value *appended = mp_value_new(MP_TYPE_INT, 44100);

	zassert_ok(mp_structure_append(&si, MP_CAPS_SAMPLE_RATE, appended), "append failed");

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

	struct mp_value *retrieved = mp_structure_get_value(&si, MP_CAPS_SAMPLE_RATE);

	zassert_not_null(retrieved, "appended field not found");
	zassert_equal(mp_value_get_int(retrieved), 44100, "retrieved value != 44100");
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

ZTEST(mp_structure_api, test_intersect)
{
	struct mp_structure *s1 =
		mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE, MP_TYPE_INT_RANGE, 8000,
				 48000, 8000, MP_STRUCTURE_END);
	struct mp_structure *s2 = mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE,
						   MP_TYPE_INT, 16000, MP_STRUCTURE_END);

	zassert_true(mp_structure_can_intersect(s1, s2), "structures cannot intersect");

	struct mp_structure *result = mp_structure_intersect(s1, s2);

	zassert_not_null(result, "intersection returned NULL");
	zassert_true(mp_structure_is_fixed(result), "intersection result not fixed");

	mp_structure_destroy(s1);
	mp_structure_destroy(s2);
	mp_structure_destroy(result);
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

	struct mp_structure *audio = mp_structure_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_SAMPLE_RATE,
						      MP_TYPE_INT, 48000, MP_STRUCTURE_END);
	struct mp_structure *video = mp_structure_new(MP_MEDIA_VIDEO, MP_CAPS_IMAGE_WIDTH,
						      MP_TYPE_INT, 1920, MP_STRUCTURE_END);

	zassert_false(mp_structure_can_intersect(audio, video),
		      "different media types can intersect");

	mp_structure_destroy(audio);
	mp_structure_destroy(video);
}
