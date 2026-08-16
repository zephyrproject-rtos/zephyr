/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The transform's caps walk, mpipe_transform.c.
 *
 * Three enumerations nest here: this pad's candidates, the element's mappings
 * of one candidate onto the other side, and the mapping of the peer's answer
 * back again. The peer is the only place the walk is visible from outside, so
 * the fake below records every capability it is handed - that sequence is the
 * enumeration order, which is what makes the ordering assertions possible.
 *
 * Capabilities carry a single unsigned field, so an intersection either matches
 * exactly or fails, and nothing in a test turns on how fields merge.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/mpipe_value.h>

/* Sink-side capabilities */
#define CAP_A 0xA1U
#define CAP_B 0xB2U
/* Source-side capabilities */
#define CAP_X 0x11U
#define CAP_Y 0x22U
#define CAP_Z 0x33U

/* One mapping the element can perform, read in either direction */
struct fake_map {
	uint32_t sink;
	uint32_t src;
};

struct mpipe_transform_api_fixture {
	struct mpipe_transform transform;
	struct mpipe_pad peer;

	/* What the element can do */
	const struct fake_map *maps;
	size_t maps_len;
	const uint32_t *candidates;
	size_t candidates_len;

	/* The one source-side capability the peer accepts, 0 to refuse all */
	uint32_t accept;
	/* What it replies with, 0 to echo what it was offered */
	uint32_t answer;

	/* Every capability the peer was offered, in order */
	uint32_t offered[8];
	size_t offered_len;

	/* transform_caps behaviours the walk has to cope with */
	bool eagain_at_zero;
	bool always_eagain;
};

/*
 * The fakes are pad and element hooks, so they cannot reach the ztest fixture
 * through their arguments. There is one transform per test, so they read the
 * fixture storage directly.
 */
static struct mpipe_transform_api_fixture test_state;

static void caps_of(struct mpipe_structure *out, uint32_t v)
{
	struct mpipe_value value = {.type = MPIPE_TYPE_UINT, .v_uint = v};

	zassert_ok(mpipe_structure_init(out, MPIPE_MEDIA_VIDEO), "structure init failed");
	zassert_ok(mpipe_structure_append_value(out, MPIPE_CAPS_PIXEL_FORMAT, &value),
		   "append failed");
}

static uint32_t value_of(const struct mpipe_structure *caps)
{
	const struct mpipe_value *v = mpipe_structure_get_value(caps, MPIPE_CAPS_PIXEL_FORMAT);

	return (v == NULL) ? 0U : mpipe_value_get_uint(v);
}

/* The candidates this side offers, narrowed by the query's filter */
static int fake_enum_caps(struct mpipe_pad *pad, uint32_t index,
			  const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	struct mpipe_structure candidate;

	ARG_UNUSED(pad);

	if (index >= test_state.candidates_len) {
		return -ENOENT;
	}

	caps_of(&candidate, test_state.candidates[index]);

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

/*
 * Produce the index-th mapping of @p in onto the @p direction side. The table is
 * read sink-to-source or source-to-sink depending on the side asked for, so the
 * same table also maps the peer's answer back.
 */
static int fake_transform_caps(struct mpipe_transform *self, enum mpipe_pad_direction direction,
			       const struct mpipe_structure *in, uint32_t index,
			       struct mpipe_structure *out)
{
	uint32_t want = value_of(in);
	uint32_t matched = 0;

	ARG_UNUSED(self);

	/* Never reports the end: the walk has to stop itself */
	if (test_state.always_eagain) {
		return -EAGAIN;
	}

	/* This index produces nothing, but a later one may */
	if (test_state.eagain_at_zero) {
		if (index == 0U) {
			return -EAGAIN;
		}
		index--;
	}

	for (size_t i = 0; i < test_state.maps_len; i++) {
		bool to_src = (direction == MPIPE_PAD_SRC);
		uint32_t from = to_src ? test_state.maps[i].sink : test_state.maps[i].src;
		uint32_t to = to_src ? test_state.maps[i].src : test_state.maps[i].sink;

		if (from != want) {
			continue;
		}

		if (matched == index) {
			caps_of(out, to);
			return 0;
		}

		matched++;
	}

	return -ENOENT;
}

/* Records what it is offered, then accepts the one capability it likes */
static int fake_peer_query(struct mpipe_pad *pad, struct mpipe_dispatch *query)
{
	uint32_t offered = value_of(query->caps);

	ARG_UNUSED(pad);

	if (test_state.offered_len < ARRAY_SIZE(test_state.offered)) {
		test_state.offered[test_state.offered_len] = offered;
	}
	test_state.offered_len++;

	if (offered != test_state.accept) {
		return -ENODATA;
	}

	if (test_state.answer != 0U) {
		caps_of(query->caps, test_state.answer);
	}

	/* Otherwise accepted as offered: query->caps already holds the answer */
	return 0;
}

/* Ask the transform's sink pad, the way an upstream element would */
static int query_sink(struct mpipe_structure *caps)
{
	struct mpipe_dispatch query = {.type = MPIPE_DISPATCH_CAPS, .caps = caps};

	return mpipe_pad_query(&test_state.transform.sink_pad, &query);
}

static void *transform_suite_setup(void)
{
	return &test_state;
}

static void transform_before(void *f)
{
	struct mpipe_transform_api_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));

	zassert_ok(mpipe_transform_init(&fix->transform, 0), "transform init failed");
	mpipe_pad_init(&fix->peer, 1, MPIPE_PAD_SINK, MPIPE_PAD_ALWAYS);

	fix->transform.sink_pad.enum_caps_fn = fake_enum_caps;
	fix->transform.transform_caps = fake_transform_caps;
	fix->peer.query_fn = fake_peer_query;

	zassert_ok(mpipe_pad_link(&fix->transform.src_pad, &fix->peer), "link failed");
}

ZTEST_SUITE(mpipe_transform_api, NULL, transform_suite_setup, transform_before, NULL, NULL);

/* A -> {X, Y}, B -> {Z}, and the peer takes only Y */
static const struct fake_map two_candidate_maps[] = {
	{CAP_A, CAP_X},
	{CAP_A, CAP_Y},
	{CAP_B, CAP_Z},
};
static const uint32_t two_candidates[] = {CAP_A, CAP_B};

static void use_two_candidate_maps(struct mpipe_transform_api_fixture *fix)
{
	fix->maps = two_candidate_maps;
	fix->maps_len = ARRAY_SIZE(two_candidate_maps);
	fix->candidates = two_candidates;
	fix->candidates_len = ARRAY_SIZE(two_candidates);
}

/*
 * Every mapping of a candidate is offered, in index order, exactly once, and
 * the walk stops at the first the peer takes. An index skipped or replayed by
 * the hand-off between mpipe_transform_enum_caps() and its callers shows up
 * here as a wrong sequence.
 */
ZTEST_F(mpipe_transform_api, test_every_mapping_is_offered_once_in_order)
{
	struct mpipe_structure caps;

	use_two_candidate_maps(fixture);
	fixture->accept = CAP_Y;

	zassert_ok(mpipe_structure_init_any(&caps), "filter init failed");
	zassert_ok(query_sink(&caps), "the query should have been answered");

	zassert_equal(fixture->offered_len, 2, "the peer saw %zu offers, expected 2",
		      fixture->offered_len);
	zassert_equal(fixture->offered[0], CAP_X, "A's first mapping was not offered first");
	zassert_equal(fixture->offered[1], CAP_Y, "A's second mapping was not offered second");

	/* The asker gets the candidate back, not the peer's reply */
	zassert_equal(value_of(&caps), CAP_A, "the answer was not the candidate");

	/* The peer's reply is kept on the pad it came from, for the caps event */
	zassert_equal(value_of(&fixture->transform.src_pad.caps), CAP_Y,
		      "the peer's answer was not published on the source pad");
}

/*
 * Nothing accepted: every mapping of every candidate is tried, the query fails,
 * and the source pad is left exactly as it was found. mpipe_transform_offer()
 * publishes to that pad only once an attempt has succeeded.
 */
ZTEST_F(mpipe_transform_api, test_a_refused_negotiation_leaves_the_pads_alone)
{
	struct mpipe_structure caps;

	use_two_candidate_maps(fixture);
	fixture->accept = 0U;

	zassert_ok(mpipe_structure_init_any(&caps), "filter init failed");
	zassert_equal(query_sink(&caps), -ENODATA, "a fully refused query should report -ENODATA");

	zassert_equal(fixture->offered_len, 3, "the peer saw %zu offers, expected 3",
		      fixture->offered_len);
	zassert_equal(fixture->offered[2], CAP_Z, "the walk did not move on to the next candidate");

	zassert_true(mpipe_structure_is_any(&fixture->transform.src_pad.caps),
		     "a refused negotiation left a capability on the source pad");
}

/*
 * -EAGAIN means this index produces nothing but a later one may. It is part of
 * the documented transform_caps contract and no element in the tree returns it,
 * so this is the only thing holding the skip up.
 */
ZTEST_F(mpipe_transform_api, test_an_index_producing_nothing_is_skipped)
{
	static const struct fake_map maps[] = {{CAP_A, CAP_X}};
	static const uint32_t candidates[] = {CAP_A};
	struct mpipe_structure caps;

	fixture->maps = maps;
	fixture->maps_len = ARRAY_SIZE(maps);
	fixture->candidates = candidates;
	fixture->candidates_len = ARRAY_SIZE(candidates);
	fixture->accept = CAP_X;
	fixture->eagain_at_zero = true;

	zassert_ok(mpipe_structure_init_any(&caps), "filter init failed");
	zassert_ok(query_sink(&caps), "an -EAGAIN index should be stepped over");

	zassert_equal(fixture->offered_len, 1, "the peer saw %zu offers, expected 1",
		      fixture->offered_len);
	zassert_equal(fixture->offered[0], CAP_X, "the mapping after -EAGAIN was not offered");
	zassert_equal(value_of(&caps), CAP_A, "the answer was not the candidate");
}

/*
 * An element that never reports the end must not spin forever. The walk gives
 * up past UINT16_MAX and reports it, which is the same guard mpipe_pad_enum_caps()
 * applies to a pad. This case hangs rather than fails if the guard is removed.
 */
ZTEST_F(mpipe_transform_api, test_a_mapping_that_never_ends_terminates)
{
	static const uint32_t candidates[] = {CAP_A};
	struct mpipe_structure caps;

	fixture->candidates = candidates;
	fixture->candidates_len = ARRAY_SIZE(candidates);
	fixture->always_eagain = true;

	zassert_ok(mpipe_structure_init_any(&caps), "filter init failed");
	zassert_equal(query_sink(&caps), -ENODATA,
		      "a mapping that never ends should exhaust the candidate");

	zassert_equal(fixture->offered_len, 0, "nothing should have reached the peer");
}

/*
 * An element with no capability at all cannot answer any query, which is a
 * caller error; one whose candidates were all refused simply has no answer.
 * The distinction mirrors mpipe_src_negotiate().
 */
ZTEST_F(mpipe_transform_api, test_no_capability_at_all_is_a_caller_error)
{
	struct mpipe_structure caps;

	fixture->candidates_len = 0;

	zassert_ok(mpipe_structure_init_any(&caps), "filter init failed");
	zassert_equal(query_sink(&caps), -EINVAL,
		      "an element with no capability should report -EINVAL");
	zassert_equal(fixture->offered_len, 0, "nothing should have reached the peer");
}

/*
 * The peer accepts, but answers something that maps back to a different
 * candidate. That is a refusal as far as this candidate is concerned, and the
 * pad must be left alone: mpipe_transform_offer() publishes the answer only
 * after the narrowing has succeeded.
 */
ZTEST_F(mpipe_transform_api, test_an_answer_that_leads_elsewhere_publishes_nothing)
{
	struct mpipe_structure caps;

	use_two_candidate_maps(fixture);

	/* Offer A's mappings, accept X, but reply with Z - which maps back to B */
	fixture->candidates_len = 1;
	fixture->accept = CAP_X;
	fixture->answer = CAP_Z;

	zassert_ok(mpipe_structure_init_any(&caps), "filter init failed");
	zassert_equal(query_sink(&caps), -ENODATA,
		      "an answer outside the candidate should not be accepted");

	zassert_true(mpipe_structure_is_any(&fixture->transform.src_pad.caps),
		     "an answer that led nowhere was published on the source pad");
}

/*
 * Two candidates mapping to the same capability: the peer's answer maps back to
 * both, and only the one the attempt started from may win. Without the walk in
 * mpipe_transform_narrow_to_candidate() the first back-mapping would be taken
 * and the wrong candidate answered.
 */
ZTEST_F(mpipe_transform_api, test_the_answer_maps_back_to_the_candidate_it_started_from)
{
	static const struct fake_map maps[] = {{CAP_A, CAP_X}, {CAP_B, CAP_X}};
	static const uint32_t candidates[] = {CAP_B};
	struct mpipe_structure caps;

	fixture->maps = maps;
	fixture->maps_len = ARRAY_SIZE(maps);
	fixture->candidates = candidates;
	fixture->candidates_len = ARRAY_SIZE(candidates);
	fixture->accept = CAP_X;

	zassert_ok(mpipe_structure_init_any(&caps), "filter init failed");
	zassert_ok(query_sink(&caps), "the query should have been answered");

	zassert_equal(fixture->offered_len, 1, "the peer saw %zu offers, expected 1",
		      fixture->offered_len);
	zassert_equal(value_of(&caps), CAP_B,
		      "the answer was the first back-mapping, not the candidate");
}

/* A filter narrows which candidates are offered at all */
ZTEST_F(mpipe_transform_api, test_the_filter_selects_the_candidate)
{
	struct mpipe_structure caps;

	use_two_candidate_maps(fixture);
	fixture->accept = CAP_Z;

	/* Ask for B only: A's mappings must never be offered */
	caps_of(&caps, CAP_B);
	zassert_ok(query_sink(&caps), "the query should have been answered");

	zassert_equal(fixture->offered_len, 1, "the peer saw %zu offers, expected 1",
		      fixture->offered_len);
	zassert_equal(fixture->offered[0], CAP_Z, "the filtered-out candidate was offered");
	zassert_equal(value_of(&caps), CAP_B, "the answer was not the filtered candidate");
}
