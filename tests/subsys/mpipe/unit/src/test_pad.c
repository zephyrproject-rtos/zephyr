/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

struct mpipe_pad_api_fixture {
	struct mpipe_pad src_pad;
	struct mpipe_pad sink_pad;
};

static void *pad_suite_setup(void)
{
	static struct mpipe_pad_api_fixture fixture;

	return &fixture;
}

static void pad_before(void *f)
{
	struct mpipe_pad_api_fixture *fix = f;

	memset(&fix->src_pad, 0, sizeof(fix->src_pad));
	memset(&fix->sink_pad, 0, sizeof(fix->sink_pad));

	mpipe_pad_init(&fix->src_pad, 0, MPIPE_PAD_SRC, MPIPE_PAD_ALWAYS);
	mpipe_pad_init(&fix->sink_pad, 1, MPIPE_PAD_SINK, MPIPE_PAD_ALWAYS);

	zassert_equal(fix->src_pad.object.id, 0, "id != 0 after init");
	zassert_equal(fix->src_pad.direction, MPIPE_PAD_SRC, "direction != SRC");
	zassert_equal(fix->sink_pad.direction, MPIPE_PAD_SINK, "direction != SINK");
	zassert_equal(fix->src_pad.presence, MPIPE_PAD_ALWAYS, "presence != ALWAYS");
	zassert_true(mpipe_structure_is_any(&fix->src_pad.caps), "caps not ANY after init");
	zassert_is_null(fix->src_pad.peer, "peer != NULL after init");
	zassert_equal(fix->src_pad.mode, MPIPE_PAD_MODE_NONE, "mode != NONE after init");
}

static void pad_after(void *f)
{
	struct mpipe_pad_api_fixture *fix = f;

	mpipe_structure_clear(&fix->src_pad.caps);
	mpipe_structure_clear(&fix->sink_pad.caps);
}

ZTEST_SUITE(mpipe_pad_api, NULL, pad_suite_setup, pad_before, pad_after, NULL);

ZTEST_F(mpipe_pad_api, test_link_sets_peers)
{
	zassert_ok(mpipe_pad_link(&fixture->src_pad, &fixture->sink_pad), "mpipe_pad_link failed");
	zassert_equal(fixture->src_pad.peer, &fixture->sink_pad, "src_pad peer != sink_pad");
	zassert_equal(fixture->sink_pad.peer, &fixture->src_pad, "sink_pad peer != src_pad");
}

/* A pad with no hook has no answer to give, which is a state and not an error */
ZTEST_F(mpipe_pad_api, test_a_pad_without_a_hook_answers_nothing)
{
	struct mpipe_dispatch query = {.type = MPIPE_DISPATCH_CAPS};
	struct mpipe_dispatch event = {.type = MPIPE_DISPATCH_EOS};
	struct mpipe_structure caps;

	zassert_ok(mpipe_structure_init_any(&caps), "caps init failed");
	query.caps = &caps;

	fixture->src_pad.query_fn = NULL;
	zassert_equal(mpipe_pad_query(&fixture->src_pad, &query), -ENOTSUP,
		      "a pad with no query hook did not report -ENOTSUP");

	fixture->src_pad.event_fn = NULL;
	zassert_equal(mpipe_pad_send_event(&fixture->src_pad, &event), -ENOTSUP,
		      "a pad with no event hook did not report -ENOTSUP");
}

static bool caps_query_fn_ran;

/* Answers a caps query the way a real one does, by writing into the storage it
 * carries. Dereferences it unguarded, which is what the refusal below protects.
 */
static int fake_caps_query_fn(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	struct mpipe_value bitwidth = {.type = MPIPE_TYPE_UINT, .v_uint = 16U};

	ARG_UNUSED(pad);

	caps_query_fn_ran = true;

	if (mpipe_structure_init(query->caps, MPIPE_MEDIA_AUDIO_PCM) != 0) {
		return -EINVAL;
	}

	return mpipe_structure_append_value(query->caps, MPIPE_CAPS_BITWIDTH, &bitwidth);
}

ZTEST_F(mpipe_pad_api, test_caps_query_needs_storage_to_answer)
{
	struct mpipe_dispatch query = {.type = MPIPE_DISPATCH_CAPS, .caps = NULL};
	struct mpipe_structure caps;

	caps_query_fn_ran = false;
	fixture->src_pad.query_fn = fake_caps_query_fn;

	/* A caps query is answered into the storage it carries, so one that
	 * carries none is refused before the query function can dereference it.
	 */
	zassert_equal(mpipe_pad_query(&fixture->src_pad, &query), -EINVAL,
		      "caps query with no storage != -EINVAL");
	zassert_false(caps_query_fn_ran, "query function ran on a caps query with no storage");

	/* The same query, given storage, reaches the query function */
	query.caps = &caps;
	zassert_ok(mpipe_pad_query(&fixture->src_pad, &query), "caps query with storage failed");
	zassert_true(caps_query_fn_ran, "query function did not run");
	zassert_equal(mpipe_value_get_uint(mpipe_structure_get_value(&caps, MPIPE_CAPS_BITWIDTH)),
		      16U, "the answer did not reach the caller's storage");
}

/* Reports two capabilities, distinguished by their bit width */
static int fake_enum_caps(struct mpipe_pad *pad, uint32_t index,
			  const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	static const uint32_t widths[] = {16U, 24U};
	struct mpipe_structure candidate;
	struct mpipe_value value;
	int ret;

	ARG_UNUSED(pad);

	if (index >= ARRAY_SIZE(widths)) {
		return -ENOENT;
	}

	mpipe_structure_init(&candidate, MPIPE_MEDIA_AUDIO_PCM);
	value.type = MPIPE_TYPE_UINT;
	value.v_uint = widths[index];
	zassert_ok(mpipe_structure_append_value(&candidate, MPIPE_CAPS_BITWIDTH, &value),
		   "append failed");

	if (filter == NULL) {
		*out = candidate;
		return 0;
	}

	ret = mpipe_structure_intersect(&candidate, filter, out);

	return (ret != 0) ? -EAGAIN : 0;
}

ZTEST_F(mpipe_pad_api, test_enum_caps)
{
	struct mpipe_structure out;
	struct mpipe_structure filter;
	struct mpipe_value bitwidth = {.type = MPIPE_TYPE_UINT};

	/* The default produces the pad's own capability, which is ANY here */
	zassert_ok(mpipe_pad_enum_caps(&fixture->src_pad, 0, NULL, &out), "default enum failed");
	zassert_true(mpipe_structure_is_any(&out), "ANY caps did not enumerate an ANY structure");
	zassert_equal(mpipe_pad_enum_caps(&fixture->src_pad, 1, NULL, &out), -ENOENT,
		      "ANY caps enumerated more than once");

	fixture->src_pad.enum_caps_fn = fake_enum_caps;

	zassert_ok(mpipe_pad_enum_caps(&fixture->src_pad, 1, NULL, &out), "enum 1 failed");
	zassert_equal(mpipe_value_get_uint(mpipe_structure_get_value(&out, MPIPE_CAPS_BITWIDTH)),
		      24U, "wrong capability at index 1");

	/* A filter only the second capability satisfies */
	zassert_ok(mpipe_structure_init(&filter, MPIPE_MEDIA_AUDIO_PCM), "filter init failed");
	bitwidth.v_uint = 24U;
	zassert_ok(mpipe_structure_append_value(&filter, MPIPE_CAPS_BITWIDTH, &bitwidth),
		   "filter append failed");

	zassert_equal(mpipe_pad_enum_caps(&fixture->src_pad, 0, &filter, &out), -EAGAIN,
		      "unmatched index != -EAGAIN");
	zassert_ok(mpipe_pad_enum_caps(&fixture->src_pad, 1, &filter, &out), "matching 1 failed");

	/* The search skips what does not match and stops at what does */
	zassert_ok(mpipe_pad_enum_first(&fixture->src_pad, &filter, &out), "enum_first failed");
	zassert_equal(mpipe_value_get_uint(mpipe_structure_get_value(&out, MPIPE_CAPS_BITWIDTH)),
		      24U, "enum_first picked the wrong capability");

	/* No filter means the first capability wins */
	zassert_ok(mpipe_pad_enum_first(&fixture->src_pad, NULL, &out), "enum_first(NULL) failed");
	zassert_equal(mpipe_value_get_uint(mpipe_structure_get_value(&out, MPIPE_CAPS_BITWIDTH)),
		      16U, "enum_first(NULL) did not pick index 0");

	/* A filter nothing satisfies exhausts the enumeration */
	zassert_ok(mpipe_structure_init(&filter, MPIPE_MEDIA_AUDIO_PCM), "filter re-init failed");
	bitwidth.v_uint = 32U;
	zassert_ok(mpipe_structure_append_value(&filter, MPIPE_CAPS_BITWIDTH, &bitwidth),
		   "filter re-append failed");
	zassert_equal(mpipe_pad_enum_first(&fixture->src_pad, &filter, &out), -ENODATA,
		      "unsatisfiable filter != -ENODATA");

	fixture->src_pad.enum_caps_fn = NULL;
	zassert_equal(mpipe_pad_enum_caps(&fixture->src_pad, 0, NULL, &out), -EINVAL,
		      "enum without a hook != -EINVAL");
}
