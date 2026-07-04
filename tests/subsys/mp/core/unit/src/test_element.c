/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/mp/core/mp_bin.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/core/mp_sink.h>
#include <zephyr/mp/core/mp_src.h>

extern struct k_heap _system_heap;

struct mp_element_api_fixture {
	struct mp_pipeline pipeline;
	struct mp_element src;
	struct mp_element sink;
	struct mp_pad src_pad;
	struct mp_pad sink_pad;
	struct sys_memory_stats mem_before;
};

static void *element_suite_setup(void)
{
	static struct mp_element_api_fixture fixture;

	return &fixture;
}

static void element_before(void *f)
{
	struct mp_element_api_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));
	mp_element_init(&fix->src, 1);
	mp_element_init(&fix->sink, 2);
	mp_pad_init(&fix->src_pad, 0, MP_PAD_SRC, MP_PAD_ALWAYS, NULL);
	mp_pad_init(&fix->sink_pad, 1, MP_PAD_SINK, MP_PAD_ALWAYS, NULL);

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->mem_before);

	fix->src_pad.caps = mp_caps_new_any();
	fix->sink_pad.caps = mp_caps_new_any();

	zassert_equal(fix->src.object.id, 1, "Element ID shall be set by init");
	zassert_equal(fix->src.current_state, MP_STATE_READY,
		      "Element shall start in READY state after init");
	zassert_equal(fix->src.pending_state, MP_STATE_READY,
		      "pending_state shall be READY after init");
	zassert_true(sys_dlist_is_empty(&fix->src.srcpads),
		     "srcpads list shall be empty after init");
	zassert_true(sys_dlist_is_empty(&fix->sink.sinkpads),
		     "sinkpads list shall be empty after init");
}

static void element_after(void *f)
{
	struct mp_element_api_fixture *fix = f;
	struct sys_memory_stats mem_after;

	mp_caps_unref(fix->src_pad.caps);
	mp_caps_unref(fix->sink_pad.caps);

	sys_heap_runtime_stats_get(&_system_heap.heap, &mem_after);
	zassert_equal(fix->mem_before.allocated_bytes, mem_after.allocated_bytes,
		      "Memory leak detected: before=%zu after=%zu", fix->mem_before.allocated_bytes,
		      mem_after.allocated_bytes);
}

ZTEST_SUITE(mp_element_api, NULL, element_suite_setup, element_before, element_after, NULL);

ZTEST_F(mp_element_api, test_link_two_elements)
{

	/* Add both pads and verify they are registered on the element */
	mp_element_add_pad(&fixture->src, &fixture->src_pad);
	zassert_false(sys_dlist_is_empty(&fixture->src.srcpads),
		      "srcpads shall not be empty after adding src pad");
	zassert_equal(fixture->src_pad.object.container, (struct mp_object *)&fixture->src,
		      "Pad container shall reference the owning element");

	mp_element_add_pad(&fixture->sink, &fixture->sink_pad);
	zassert_false(sys_dlist_is_empty(&fixture->sink.sinkpads),
		      "sinkpads shall not be empty after adding sink pad");
	zassert_equal(fixture->sink_pad.object.container, (struct mp_object *)&fixture->sink,
		      "Pad container shall reference the owning element");

	/* src/sink are already initialised by element_before via MP_ELEMENT_INIT */
	zassert_ok(mp_element_link(&fixture->src, &fixture->sink, NULL),
		   "Linking src and sink elements shall succeed");
	zassert_true(fixture->sink_pad.peer == &fixture->src_pad,
		     "sink pad peer must be linked to src pad");
	zassert_true(fixture->src_pad.peer == &fixture->sink_pad,
		     "src pad peer must be linked to sink pad");
}

ZTEST_F(mp_element_api, test_sanity)
{
	/* get_bus: child element inside pipeline shall return the pipeline bus */
	struct mp_element child;

	MP_ELEMENT_INIT((struct mp_element *)&fixture->pipeline, mp_pipeline_init, 0);
	mp_element_init(&child, 1);

	zassert_ok(
		mp_bin_add((struct mp_bin *)&fixture->pipeline, (struct mp_element *)&child, NULL),
		"mp_bin_add shall succeed");

	struct mp_bus *bus = mp_element_get_bus((struct mp_element *)&fixture->pipeline);

	zassert_not_null(bus, "Child element shall find the pipeline bus");
	zassert_equal(bus, &fixture->pipeline.bin.bus, "Bus shall be the pipeline's bus");

	/* Unknown direction pad shall not appear in either list */
	struct mp_element elem;
	struct mp_pad pad;

	memset(&elem, 0, sizeof(elem));
	memset(&pad, 0, sizeof(pad));
	mp_element_init(&elem, 1);

	mp_pad_init(&pad, 0, MP_PAD_UNKNOWN, MP_PAD_ALWAYS, NULL);
	mp_element_add_pad(&elem, &pad);

	zassert_true(sys_dlist_is_empty(&elem.srcpads),
		     "srcpads shall remain empty for UNKNOWN direction pad");
	zassert_true(sys_dlist_is_empty(&elem.sinkpads),
		     "sinkpads shall remain empty for UNKNOWN direction pad");

	/* Linking elements without pads shall fail */
	struct mp_element src, sink;

	memset(&src, 0, sizeof(src));
	memset(&sink, 0, sizeof(sink));
	mp_element_init(&src, 1);
	mp_element_init(&sink, 2);

	zassert_true(mp_element_link(&src, &sink, NULL) < 0,
		     "Linking elements without pads shall fail");

	/* get_bus on NULL shall return NULL */
	zassert_is_null(mp_element_get_bus(NULL), "mp_element_get_bus(NULL) shall return NULL");
}
