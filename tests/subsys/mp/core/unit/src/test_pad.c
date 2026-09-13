/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>

struct mp_pad_api_fixture {
	struct mp_pad src_pad;
	struct mp_pad sink_pad;
	struct mp_caps *any_caps;
};

static void *pad_suite_setup(void)
{
	static struct mp_pad_api_fixture fixture;

	return &fixture;
}

static void pad_before(void *f)
{
	struct mp_pad_api_fixture *fix = f;

	memset(&fix->src_pad, 0, sizeof(fix->src_pad));
	memset(&fix->sink_pad, 0, sizeof(fix->sink_pad));

	fix->any_caps = mp_caps_new_any();
	mp_pad_init(&fix->src_pad, 0, MP_PAD_SRC, MP_PAD_ALWAYS, fix->any_caps);
	mp_pad_init(&fix->sink_pad, 1, MP_PAD_SINK, MP_PAD_ALWAYS, fix->any_caps);

	zassert_equal(fix->src_pad.object.id, 0, "id != 0 after init");
	zassert_equal(fix->src_pad.direction, MP_PAD_SRC, "direction != SRC");
	zassert_equal(fix->sink_pad.direction, MP_PAD_SINK, "direction != SINK");
	zassert_equal(fix->src_pad.presence, MP_PAD_ALWAYS, "presence != ALWAYS");
	zassert_equal(fix->src_pad.caps, fix->any_caps, "caps mismatch after init");
	zassert_is_null(fix->src_pad.peer, "peer != NULL after init");
	zassert_equal(fix->src_pad.mode, MP_PAD_MODE_NONE, "mode != NONE after init");
}

static void pad_after(void *f)
{
	struct mp_pad_api_fixture *fix = f;

	if (fix->any_caps) {
		mp_caps_unref(fix->src_pad.caps);
		mp_caps_unref(fix->sink_pad.caps);
		mp_caps_unref(fix->any_caps);
	}
}

ZTEST_SUITE(mp_pad_api, NULL, pad_suite_setup, pad_before, pad_after, NULL);

ZTEST_F(mp_pad_api, test_new)
{
	struct mp_pad *pad = mp_pad_new(5, MP_PAD_SINK, MP_PAD_SOMETIMES, fixture->any_caps);

	zassert_not_null(pad, "mp_pad_new returned NULL");
	zassert_equal(pad->object.id, 5, "id != 5");
	zassert_equal(pad->direction, MP_PAD_SINK, "direction != SINK");
	zassert_equal(pad->presence, MP_PAD_SOMETIMES, "presence != SOMETIMES");
	zassert_equal(pad->caps, fixture->any_caps, "caps mismatch");
	mp_caps_unref(pad->caps);
	k_free(pad);

	struct mp_pad *pad_null_caps = mp_pad_new(0, MP_PAD_SRC, MP_PAD_ALWAYS, NULL);

	zassert_not_null(pad_null_caps, "mp_pad_new(NULL caps) returned NULL");
	zassert_is_null(pad_null_caps->caps, "caps != NULL");
	k_free(pad_null_caps);
}

ZTEST_F(mp_pad_api, test_link_sets_peers)
{
	zassert_ok(mp_pad_link(&fixture->src_pad, &fixture->sink_pad), "mp_pad_link failed");
	zassert_equal(fixture->src_pad.peer, &fixture->sink_pad, "src_pad peer != sink_pad");
	zassert_equal(fixture->sink_pad.peer, &fixture->src_pad, "sink_pad peer != src_pad");
}

ZTEST_F(mp_pad_api, test_sanity)
{
	zassert_true(mp_pad_link(NULL, &fixture->sink_pad) < 0, "link(NULL, sink) did not fail");
	zassert_true(mp_pad_link(&fixture->src_pad, NULL) < 0, "link(src, NULL) did not fail");
	zassert_true(mp_pad_link(NULL, NULL) < 0, "link(NULL, NULL) did not fail");

	struct mp_dispatch evt;

	mp_dispatch_eos_init(&evt);

	zassert_true(mp_pad_send_event(NULL, &evt) < 0, "send_event(NULL, evt) did not fail");
	zassert_true(mp_pad_send_event(&fixture->src_pad, NULL) < 0,
		     "send_event(pad, NULL) did not fail");

	mp_dispatch_clear(&evt);

	struct mp_dispatch q;

	mp_dispatch_caps_init(&q, NULL);

	zassert_true(mp_pad_query(NULL, &q) < 0, "query(NULL, q) did not fail");
	zassert_true(mp_pad_query(&fixture->src_pad, NULL) < 0, "query(pad, NULL) did not fail");

	fixture->src_pad.queryfn = NULL;
	zassert_true(mp_pad_query(&fixture->src_pad, &q) < 0, "query(no queryfn) did not fail");

	mp_dispatch_clear(&q);
}
