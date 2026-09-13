/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/mp/core/mp_bin.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/core/mp_src.h>
#include <zephyr/mp/core/mp_sink.h>

struct mp_bin_api_fixture {
	struct mp_bin bin;
	struct mp_pipeline pipeline;
	struct mp_sink sink;
	struct mp_src src;
};

static void *bin_suite_setup(void)
{
	static struct mp_bin_api_fixture fixture;

	return &fixture;
}

static void bin_before(void *f)
{
	struct mp_bin_api_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));
	MP_ELEMENT_INIT((struct mp_element *)&fix->bin, mp_bin_init, 0);
	MP_ELEMENT_INIT((struct mp_element *)&fix->src, mp_src_init, 1);
	MP_ELEMENT_INIT((struct mp_element *)&fix->sink, mp_sink_init, 2);

	zassert_equal(fix->bin.children_num, 0, "children_num != 0 after init");
	zassert_true(sys_dlist_is_empty(&fix->bin.children), "children list not empty after init");
	zassert_equal(((struct mp_element *)&fix->bin)->current_state, MP_STATE_READY,
		      "state != READY after init");
}

ZTEST_SUITE(mp_bin_api, NULL, bin_suite_setup, bin_before, NULL, NULL);

ZTEST_F(mp_bin_api, test_add_elements)
{
	zassert_ok(mp_bin_add(&fixture->bin, NULL), "mp_bin_add(NULL) failed");
	zassert_equal(fixture->bin.children_num, 0, "children_num != 0");

	zassert_ok(mp_bin_add(&fixture->bin, (struct mp_element *)&fixture->src, NULL),
		   "mp_bin_add(src) failed");
	zassert_equal(fixture->bin.children_num, 1, "children_num != 1");
	zassert_equal(fixture->src.element.object.container, (struct mp_object *)&fixture->bin,
		      "src container != bin");

	zassert_ok(mp_bin_add(&fixture->bin, (struct mp_element *)&fixture->sink, NULL),
		   "mp_bin_add(sink) failed");
	zassert_equal(fixture->bin.children_num, 2, "children_num != 2");

	sys_dnode_t *first = sys_dlist_peek_head(&fixture->bin.children);

	zassert_not_null(first, "children list is empty");

	struct mp_element *first_elem = CONTAINER_OF(first, struct mp_element, object.node);

	zassert_equal(first_elem->object.id, 1, "first child id != 1");

	struct mp_src dup_src;

	memset(&dup_src, 0, sizeof(dup_src));
	MP_ELEMENT_INIT((struct mp_element *)&dup_src, mp_src_init, 1);

	zassert_true(mp_bin_add(&fixture->bin, (struct mp_element *)&dup_src, NULL) < 0,
		     "duplicate id add did not fail");
	zassert_equal(fixture->bin.children_num, 2, "children_num changed on failed add");
}

ZTEST_F(mp_bin_api, test_add_varargs)
{
	zassert_ok(mp_bin_add(&fixture->bin, (struct mp_element *)&fixture->src,
			      (struct mp_element *)&fixture->sink, NULL),
		   "mp_bin_add(src, sink) failed");
	zassert_equal(fixture->bin.children_num, 2, "children_num != 2");
}
