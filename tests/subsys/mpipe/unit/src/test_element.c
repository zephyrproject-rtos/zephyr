/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_sink.h>
#include <zephyr/mpipe/mpipe_src.h>

struct mpipe_element_api_fixture {
	struct mpipe pipeline;
	struct mpipe_element src;
	struct mpipe_element sink;
	struct mpipe_pad src_pad;
	struct mpipe_pad sink_pad;
};

static void *element_suite_setup(void)
{
	static struct mpipe_element_api_fixture fixture;

	return &fixture;
}

static void element_before(void *f)
{
	struct mpipe_element_api_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));
	mpipe_element_init(&fix->src, 1);
	mpipe_element_init(&fix->sink, 2);
	mpipe_pad_init(&fix->src_pad, 0, MPIPE_PAD_SRC, MPIPE_PAD_ALWAYS);
	mpipe_pad_init(&fix->sink_pad, 1, MPIPE_PAD_SINK, MPIPE_PAD_ALWAYS);

	mpipe_structure_init_any(&fix->src_pad.caps);
	mpipe_structure_init_any(&fix->sink_pad.caps);

	zassert_equal(fix->src.object.id, 1, "Element ID shall be set by init");
	zassert_equal(fix->src.current_state, MPIPE_STATE_READY,
		      "Element shall start in READY state after init");
	zassert_true(sys_dlist_is_empty(&fix->src.src_pads),
		     "src_pads list shall be empty after init");
	zassert_true(sys_dlist_is_empty(&fix->sink.sink_pads),
		     "sink_pads list shall be empty after init");
}

ZTEST_SUITE(mpipe_element_api, NULL, element_suite_setup, element_before, NULL, NULL);

ZTEST_F(mpipe_element_api, test_link_two_elements)
{

	/* Add both pads and verify they are registered on the element */
	mpipe_element_add_pad(&fixture->src, &fixture->src_pad);
	zassert_false(sys_dlist_is_empty(&fixture->src.src_pads),
		      "src_pads shall not be empty after adding src pad");
	zassert_equal(fixture->src_pad.object.container, (struct mpipe_object *)&fixture->src,
		      "Pad container shall reference the owning element");

	mpipe_element_add_pad(&fixture->sink, &fixture->sink_pad);
	zassert_false(sys_dlist_is_empty(&fixture->sink.sink_pads),
		      "sink_pads shall not be empty after adding sink pad");
	zassert_equal(fixture->sink_pad.object.container, (struct mpipe_object *)&fixture->sink,
		      "Pad container shall reference the owning element");

	/* src/sink are already initialised by element_before via their init functions */
	zassert_ok(mpipe_element_link(&fixture->src, &fixture->sink, NULL),
		   "Linking src and sink elements shall succeed");
	zassert_true(fixture->sink_pad.peer == &fixture->src_pad,
		     "sink pad peer must be linked to src pad");
	zassert_true(fixture->src_pad.peer == &fixture->sink_pad,
		     "src pad peer must be linked to sink pad");
}

/*
 * A bus lives on a bin, so a bin answers with its own and anything else with
 * the bin holding it. A nested bin is what makes "nearest" observable: its
 * children get its bus and stop there, rather than reaching the pipeline.
 */
ZTEST_F(mpipe_element_api, test_get_bus_finds_the_nearest_bin)
{
	struct mpipe_bin nested;
	struct mpipe_element child;
	struct mpipe_element orphan;

	memset(&nested, 0, sizeof(nested));
	memset(&child, 0, sizeof(child));
	memset(&orphan, 0, sizeof(orphan));

	zassert_ok(mpipe_pipeline_init(&fixture->pipeline, 0));
	zassert_ok(mpipe_bin_init(&nested, 1));
	mpipe_element_init(&child, 2);
	mpipe_element_init(&orphan, 3);

	/* Held by no bin, so there is nothing to hand out */
	zassert_is_null(mpipe_element_get_bus_chan(&orphan),
			"An element outside any bin shall have no bus");

	/* A pipeline has no container, and answers with its own bus */
	zassert_equal(mpipe_element_get_bus_chan((struct mpipe_element *)&fixture->pipeline),
		      &fixture->pipeline.bin.bus.channel,
		      "A pipeline shall answer with its own bus");

	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&fixture->pipeline,
				 (struct mpipe_element *)&nested, NULL),
		   "mpipe_bin_add shall succeed");
	zassert_ok(mpipe_bin_add(&nested, &child, NULL), "mpipe_bin_add shall succeed");

	/* A bin answers with its own bus whether or not another bin holds it */
	zassert_equal(mpipe_element_get_bus_chan((struct mpipe_element *)&nested),
		      &nested.bus.channel,
		      "A nested bin shall answer with its own bus, not its parent's");

	/* The whole point of "nearest": the walk stops at the bin holding it */
	zassert_equal(mpipe_element_get_bus_chan(&child), &nested.bus.channel,
		      "A child shall get the bus of the bin holding it");
	zassert_not_equal(mpipe_element_get_bus_chan(&child), &fixture->pipeline.bin.bus.channel,
			  "A child shall not reach past its bin to the pipeline");
}

ZTEST_F(mpipe_element_api, test_sanity)
{
	/* Unknown direction pad shall not appear in either list */
	struct mpipe_element elem;
	struct mpipe_pad pad;

	memset(&elem, 0, sizeof(elem));
	memset(&pad, 0, sizeof(pad));
	mpipe_element_init(&elem, 1);

	mpipe_pad_init(&pad, 0, MPIPE_PAD_UNKNOWN, MPIPE_PAD_ALWAYS);
	mpipe_element_add_pad(&elem, &pad);

	zassert_true(sys_dlist_is_empty(&elem.src_pads),
		     "src_pads shall remain empty for UNKNOWN direction pad");
	zassert_true(sys_dlist_is_empty(&elem.sink_pads),
		     "sink_pads shall remain empty for UNKNOWN direction pad");

	/* Linking elements without pads shall fail */
	struct mpipe_element src, sink;

	memset(&src, 0, sizeof(src));
	memset(&sink, 0, sizeof(sink));
	mpipe_element_init(&src, 1);
	mpipe_element_init(&sink, 2);

	zassert_true(mpipe_element_link(&src, &sink, NULL) < 0,
		     "Linking elements without pads shall fail");
}
