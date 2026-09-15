/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/sys_heap.h>
#include <zephyr/ztest.h>

#include <zephyr/mp/core/mp.h>
#include <zephyr/mp/core/mp_fake_src.h>
#include <zephyr/mp/core/mp_sink.h>
#include <zephyr/mp/core/mp_transform.h>

extern struct k_heap _system_heap;

#define PIPE_ID      0
#define SRC_ID       1
#define TRANSFORM_ID 2
#define SINK_ID      3

/* Number of buffers the source shall produce before EOS */
#define TEST_BUFS_NUM 10

struct test_mock_pipeline_fixture {
	struct mp_pipeline pipeline;
	struct mp_fake_src fake_src;
	struct mp_transform transform;
	struct mp_sink sink;
	struct sys_memory_stats mem_before;
};

static void *pipeline_suite_setup(void)
{
	static struct test_mock_pipeline_fixture fixture;

	return &fixture;
}

static void pipeline_before(void *f)
{
	struct test_mock_pipeline_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));

	MP_ELEMENT_INIT(&fix->pipeline, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&fix->fake_src, mp_fake_src_init, SRC_ID);
	MP_ELEMENT_INIT(&fix->transform, mp_transform_init, TRANSFORM_ID);
	MP_ELEMENT_INIT(&fix->sink, mp_sink_init, SINK_ID);

	/* Set number of buffers to produce before EOS */
	zassert_ok(mp_object_set_properties((struct mp_object *)&fix->fake_src, PROP_NUM_BUFS,
					    TEST_BUFS_NUM, PROP_LIST_END),
		   "Failed to set fake_src PROP_NUM_BUFS");

	sys_heap_runtime_stats_get(&_system_heap.heap, &fix->mem_before);
}

ZTEST_SUITE(test_mock_pipeline, NULL, pipeline_suite_setup, pipeline_before, NULL, NULL);

ZTEST_F(test_mock_pipeline, test_pipeline_fakesrc_transform_sink)
{
	struct mp_bus *bus;
	struct mp_message msg;
	struct sys_memory_stats mem_after;

	/* Add all elements to the pipeline */
	zassert_ok(mp_bin_add((struct mp_bin *)&fixture->pipeline,
			      (struct mp_element *)&fixture->fake_src,
			      (struct mp_element *)&fixture->transform,
			      (struct mp_element *)&fixture->sink, NULL),
		   "Failed to add elements");

	/* Link: fake_src → transform → sink */
	zassert_ok(mp_element_link((struct mp_element *)&fixture->fake_src,
				   (struct mp_element *)&fixture->transform,
				   (struct mp_element *)&fixture->sink, NULL),
		   "Failed to link elements");

	/* Start the pipeline */
	zassert_equal(
		mp_element_set_state((struct mp_element *)&fixture->pipeline, MP_STATE_PLAYING),
		MP_STATE_CHANGE_SUCCESS, "Pipline failed to start PLAYING");

	/* Wait for EOS posted by the sink */
	bus = mp_element_get_bus((struct mp_element *)&fixture->pipeline);
	mp_bus_pop_msg(bus, MP_MESSAGE_EOS | MP_MESSAGE_ERROR, &msg);
	zassert_equal(msg.type, MP_MESSAGE_EOS, "Expected EOS Message,  got %d", msg.type);

	/* Bring pipeline back to READY and join the thread */
	zassert_equal(mp_element_set_state((struct mp_element *)&fixture->pipeline, MP_STATE_READY),
		      MP_STATE_CHANGE_SUCCESS, "Pipeline failed to return to READY");

	/*
	 * The pad's caps hold the negotiated caps which are not automatically released upon
	 * state change to READY, unref them here to avoid the memory leak false detection.
	 */
	mp_caps_unref(fixture->fake_src.src.srcpad.caps);
	mp_caps_unref(fixture->transform.srcpad.caps);
	mp_caps_unref(fixture->transform.sinkpad.caps);
	mp_caps_unref(fixture->sink.sinkpad.caps);

	/* Check if heap memory was properly cleaned up */
	sys_heap_runtime_stats_get(&_system_heap.heap, &mem_after);
	zassert_equal(fixture->mem_before.allocated_bytes, mem_after.allocated_bytes,
		      "Memory leak detected: before=%zu after=%zu",
		      fixture->mem_before.allocated_bytes, mem_after.allocated_bytes);
}
