/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_src.h>
#include <zephyr/mpipe/mpipe_sink.h>

struct mpipe_bin_api_fixture {
	struct mpipe_bin bin;
	struct mpipe pipeline;
	struct mpipe_sink sink;
	struct mpipe_src src;
};

static void *bin_suite_setup(void)
{
	static struct mpipe_bin_api_fixture fixture;

	return &fixture;
}

static void bin_before(void *f)
{
	struct mpipe_bin_api_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));
	zassert_ok(mpipe_bin_init(&fix->bin, 0));
	zassert_ok(mpipe_src_init(&fix->src, 1));
	zassert_ok(mpipe_sink_init(&fix->sink, 2));

	zassert_equal(fix->bin.children_num, 0, "children_num != 0 after init");
	zassert_true(sys_dlist_is_empty(&fix->bin.children), "children list not empty after init");
	zassert_equal(((struct mpipe_element *)&fix->bin)->current_state, MPIPE_STATE_READY,
		      "state != READY after init");
}

ZTEST_SUITE(mpipe_bin_api, NULL, bin_suite_setup, bin_before, NULL, NULL);

ZTEST_F(mpipe_bin_api, test_add_elements)
{
	zassert_ok(mpipe_bin_add(&fixture->bin, NULL), "mpipe_bin_add(NULL) failed");
	zassert_equal(fixture->bin.children_num, 0, "children_num != 0");

	zassert_ok(mpipe_bin_add(&fixture->bin, (struct mpipe_element *)&fixture->src, NULL),
		   "mpipe_bin_add(src) failed");
	zassert_equal(fixture->bin.children_num, 1, "children_num != 1");
	zassert_equal(fixture->src.element.object.container, (struct mpipe_object *)&fixture->bin,
		      "src container != bin");

	zassert_ok(mpipe_bin_add(&fixture->bin, (struct mpipe_element *)&fixture->sink, NULL),
		   "mpipe_bin_add(sink) failed");
	zassert_equal(fixture->bin.children_num, 2, "children_num != 2");

	sys_dnode_t *first = sys_dlist_peek_head(&fixture->bin.children);

	zassert_not_null(first, "children list is empty");

	struct mpipe_element *first_elem = CONTAINER_OF(first, struct mpipe_element, object.node);

	zassert_equal(first_elem->object.id, 1, "first child id != 1");

	struct mpipe_src dup_src;

	memset(&dup_src, 0, sizeof(dup_src));
	zassert_ok(mpipe_src_init(&dup_src, 1));

	zassert_true(mpipe_bin_add(&fixture->bin, (struct mpipe_element *)&dup_src, NULL) < 0,
		     "duplicate id add did not fail");
	zassert_equal(fixture->bin.children_num, 2, "children_num changed on failed add");
}

ZTEST_F(mpipe_bin_api, test_add_varargs)
{
	zassert_ok(mpipe_bin_add(&fixture->bin, (struct mpipe_element *)&fixture->src,
				 (struct mpipe_element *)&fixture->sink, NULL),
		   "mpipe_bin_add(src, sink) failed");
	zassert_equal(fixture->bin.children_num, 2, "children_num != 2");
}
