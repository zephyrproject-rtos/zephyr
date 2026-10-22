/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/base/mpipe_app_sink.h>
#include <zephyr/mpipe/base/mpipe_app_src.h>

ZBUS_MSG_SUBSCRIBER_DEFINE(test_app_sub);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	SRC_ID,
	SINK_ID,
};

#define TEST_BUFS_NUM 6
#define TEST_PAYLOAD  (CONFIG_MPIPE_BASE_APP_SRC_BUF_SZ / 2)

BUILD_ASSERT(TEST_BUFS_NUM > CONFIG_MPIPE_BASE_APP_SINK_QUEUE_DEPTH,
	     "the pull tests need more buffers than the pull queue holds");

/* What the callback saw: one entry per delivered buffer */
static struct {
	atomic_t count;
	uint32_t bytes_used[TEST_BUFS_NUM];
	uint8_t first_byte[TEST_BUFS_NUM];
} seen;

static void app_sink_cb(const struct net_buf *buf, void *user_data)
{
	uint32_t i = (uint32_t)atomic_inc(&seen.count);

	zassert_equal_ptr(user_data, &seen, "user_data not passed through");

	if (i < TEST_BUFS_NUM) {
		seen.bytes_used[i] = mpipe_buffer_get_meta(buf)->bytes_used;
		seen.first_byte[i] = buf->data[0];
	}
}

struct test_app_fixture {
	struct mpipe pipeline;
	struct mpipe_app_src app_src;
	struct mpipe_app_sink app_sink;
	struct zbus_channel *bus;
};

static void *app_suite_setup(void)
{
	static struct test_app_fixture fixture;

	return &fixture;
}

static void app_before(void *f)
{
	struct test_app_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));
	memset(&seen, 0, sizeof(seen));

	zassert_ok(mpipe_pipeline_init(&fix->pipeline, PIPE_ID));
	zassert_ok(mpipe_app_src_init(&fix->app_src, SRC_ID));
	zassert_ok(mpipe_app_sink_init(&fix->app_sink, SINK_ID));

	zassert_ok(mpipe_bin_add((struct mpipe_bin *)&fix->pipeline,
				 (struct mpipe_element *)&fix->app_src,
				 (struct mpipe_element *)&fix->app_sink, NULL),
		   "Failed to add elements");
	zassert_ok(mpipe_element_link((struct mpipe_element *)&fix->app_src,
				      (struct mpipe_element *)&fix->app_sink, NULL),
		   "Failed to link elements");

	fix->bus = mpipe_element_get_bus_chan((struct mpipe_element *)&fix->pipeline);
	zassert_ok(zbus_chan_add_obs(fix->bus, &test_app_sub, K_FOREVER),
		   "Failed to add observer to pipeline channel");
}

static void app_after(void *f)
{
	struct test_app_fixture *fix = f;

	/* Also runs when the test failed before tearing its pipeline down */
	(void)mpipe_element_set_state((struct mpipe_element *)&fix->pipeline, MPIPE_STATE_READY);
	(void)zbus_chan_rm_obs(fix->bus, &test_app_sub, K_FOREVER);
}

ZTEST_SUITE(test_app, NULL, app_suite_setup, app_before, app_after, NULL);

/* A grey 8x8 capability, what the application declares on either end */
static void grey_caps(struct mpipe_structure *caps, uint32_t pixfmt)
{
	zassert_ok(mpipe_structure_init_fields(caps, MPIPE_MEDIA_VIDEO, MPIPE_CAPS_PIXEL_FORMAT,
					       MPIPE_TYPE_UINT, pixfmt, MPIPE_CAPS_IMAGE_WIDTH,
					       MPIPE_TYPE_UINT, 8, MPIPE_CAPS_IMAGE_HEIGHT,
					       MPIPE_TYPE_UINT, 8, MPIPE_CAPS_END),
		   "init caps failed");
}

static void set_caps(struct mpipe_app_src *app_src, struct mpipe_app_sink *app_sink)
{
	struct mpipe_structure caps;

	grey_caps(&caps, VIDEO_PIX_FMT_GREY);
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)app_src,
					       MPIPE_PROP_BASE_APP_SRC_CAPS, &caps,
					       MPIPE_PROP_LIST_END),
		   "Failed to set app_src caps");
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)app_sink,
					       MPIPE_PROP_BASE_APP_SINK_CAPS, &caps,
					       MPIPE_PROP_LIST_END),
		   "Failed to set app_sink caps");
}

static void set_cb(struct mpipe_app_sink *app_sink)
{
	static const struct mpipe_app_sink_cb cb = {.fn = app_sink_cb, .user_data = &seen};

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)app_sink,
					       MPIPE_PROP_BASE_APP_SINK_CB, &cb,
					       MPIPE_PROP_LIST_END),
		   "Failed to set app_sink callback");
}

/* Push TEST_BUFS_NUM payloads whose first byte is their index, then EOS */
static void push_all(struct mpipe_app_src *app_src)
{
	uint8_t payload[TEST_PAYLOAD];

	for (uint8_t i = 0; i < TEST_BUFS_NUM; i++) {
		memset(payload, i, sizeof(payload));
		zassert_ok(mpipe_app_src_push(app_src, payload, sizeof(payload), K_SECONDS(1)),
			   "Failed to push payload %u", i);
	}
	zassert_ok(mpipe_app_src_eos(app_src, K_SECONDS(1)), "Failed to push EOS");
}

/* One EOS and nothing else on the bus */
static void expect_eos(void)
{
	const struct zbus_channel *chan;
	struct mpipe_message msg;

	zassert_ok(zbus_sub_wait_msg(&test_app_sub, &chan, &msg, K_SECONDS(2)),
		   "timed out waiting for a pipeline message");
	zassert_equal(msg.type, MPIPE_MESSAGE_EOS, "expected EOS, got %d", msg.type);
	zassert_equal(zbus_sub_wait_msg(&test_app_sub, &chan, &msg, K_MSEC(50)), -ENOMSG,
		      "more than one message");
}

/*
 * Callback delivery, repeated: every payload reaches the callback in order with
 * its size, and the declared capability is the one negotiated on every run,
 * not only the first.
 */
ZTEST_F(test_app, test_push_callback)
{
	struct mpipe_element *pipe = (struct mpipe_element *)&fixture->pipeline;
	const struct mpipe_value *v;

	set_caps(&fixture->app_src, &fixture->app_sink);
	set_cb(&fixture->app_sink);

	for (int run = 0; run < 3; run++) {
		atomic_set(&seen.count, 0);

		zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_PLAYING),
			   "run %d failed to start PLAYING", run);

		v = mpipe_structure_get_value(&fixture->app_sink.sink.sink_pad.caps,
					      MPIPE_CAPS_PIXEL_FORMAT);
		zassert_not_null(v, "run %d: no pixel format negotiated on the sink", run);
		zassert_equal(mpipe_value_get_uint(v), VIDEO_PIX_FMT_GREY,
			      "run %d: negotiated caps differ from the property", run);

		push_all(&fixture->app_src);
		expect_eos();

		zassert_equal(atomic_get(&seen.count), TEST_BUFS_NUM,
			      "run %d: expected %d deliveries, got %ld", run, TEST_BUFS_NUM,
			      atomic_get(&seen.count));
		for (uint8_t i = 0; i < TEST_BUFS_NUM; i++) {
			zassert_equal(seen.first_byte[i], i, "run %d: payload %u out of order", run,
				      i);
			zassert_equal(seen.bytes_used[i], TEST_PAYLOAD,
				      "run %d: payload %u bytes_used", run, i);
		}

		zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_READY),
			   "run %d failed to return to READY", run);
	}
}

/* Pull delivery: the application drains the queue at its own pace and owns what it gets */
ZTEST_F(test_app, test_push_pull)
{
	struct mpipe_element *pipe = (struct mpipe_element *)&fixture->pipeline;
	uint8_t payload[TEST_PAYLOAD];
	struct net_buf *buf;

	set_caps(&fixture->app_src, &fixture->app_sink);
	zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_PLAYING));

	zassert_equal(mpipe_app_sink_pull(&fixture->app_sink, &buf, K_MSEC(20)), -EAGAIN,
		      "pull on an empty queue did not time out");

	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SINK_QUEUE_DEPTH; i++) {
		memset(payload, i, sizeof(payload));
		zassert_ok(mpipe_app_src_push(&fixture->app_src, payload, sizeof(payload),
					      K_SECONDS(1)));
	}

	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SINK_QUEUE_DEPTH; i++) {
		zassert_ok(mpipe_app_sink_pull(&fixture->app_sink, &buf, K_SECONDS(1)),
			   "pull %u failed", i);
		zassert_equal(buf->data[0], i, "pull %u out of order", i);
		zassert_equal(mpipe_buffer_get_meta(buf)->bytes_used, TEST_PAYLOAD);
		net_buf_unref(buf);
	}

	/* A puller that lags loses the arriving buffers, never the pipeline */
	push_all(&fixture->app_src);
	expect_eos();

	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SINK_QUEUE_DEPTH; i++) {
		zassert_ok(mpipe_app_sink_pull(&fixture->app_sink, &buf, K_SECONDS(1)),
			   "late pull %u failed", i);
		zassert_equal(buf->data[0], i, "late pull %u out of order", i);
		net_buf_unref(buf);
	}
	zassert_equal(mpipe_app_sink_pull(&fixture->app_sink, &buf, K_MSEC(20)), -EAGAIN,
		      "the queue held more than its depth");

	zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_READY));
}

/* The zero-copy path: fill a pool buffer in place and queue it */
ZTEST_F(test_app, test_alloc_push_buf)
{
	struct mpipe_element *pipe = (struct mpipe_element *)&fixture->pipeline;
	struct net_buf *buf;

	set_caps(&fixture->app_src, &fixture->app_sink);
	set_cb(&fixture->app_sink);
	zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_PLAYING));

	zassert_ok(mpipe_app_src_alloc(&fixture->app_src, TEST_PAYLOAD, K_NO_WAIT, &buf));
	zassert_equal(mpipe_buffer_get_meta(buf)->bytes_used, 0, "fresh buffer not empty");
	memset(buf->data, 0x5a, TEST_PAYLOAD);

	/* A size beyond the buffer is refused and the buffer stays the caller's */
	zassert_equal(mpipe_app_src_push_buf(&fixture->app_src, buf, buf->size + 1U, K_NO_WAIT),
		      -EINVAL);
	zassert_equal(buf->ref, 1, "refused push released the buffer");

	zassert_ok(mpipe_app_src_push_buf(&fixture->app_src, buf, TEST_PAYLOAD, K_SECONDS(1)));
	zassert_ok(mpipe_app_src_eos(&fixture->app_src, K_SECONDS(1)));
	expect_eos();

	zassert_equal(atomic_get(&seen.count), 1);
	zassert_equal(seen.first_byte[0], 0x5a);
	zassert_equal(seen.bytes_used[0], TEST_PAYLOAD);

	zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_READY));
}

/* What a full pool and a full queue report without waiting */
ZTEST_F(test_app, test_push_errors)
{
	struct net_buf *bufs[CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM];
	struct net_buf *extra;
	uint8_t queued = 0;
	int ret;

	/* The pipeline is READY: nothing consumes, so the pool and the queue fill up */
	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM; i++) {
		ret = mpipe_app_src_alloc(&fixture->app_src, TEST_PAYLOAD, K_NO_WAIT, &bufs[i]);
		zassert_ok(ret, "alloc %u failed", i);
	}
	zassert_equal(mpipe_app_src_alloc(&fixture->app_src, TEST_PAYLOAD, K_NO_WAIT, &extra),
		      -ENOBUFS, "an exhausted pool did not report -ENOBUFS");

	/* The queue holds at least its depth, then refuses */
	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM; i++) {
		ret = mpipe_app_src_push_buf(&fixture->app_src, bufs[i], TEST_PAYLOAD, K_NO_WAIT);
		if (ret == 0) {
			queued++;
			continue;
		}
		zassert_equal(ret, -EAGAIN, "a full queue did not report -EAGAIN");
		net_buf_unref(bufs[i]);
	}
	zassert_true(queued >= CONFIG_MPIPE_BASE_APP_SRC_QUEUE_DEPTH, "the queue held %u buffers",
		     queued);
	zassert_equal(mpipe_app_src_eos(&fixture->app_src, K_NO_WAIT), -EAGAIN,
		      "EOS on a full queue did not report -EAGAIN");

	/* Starting discards what was queued while READY; the pool is whole again */
	zassert_ok(mpipe_element_set_state((struct mpipe_element *)&fixture->pipeline,
					   MPIPE_STATE_PAUSED));
	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM; i++) {
		ret = mpipe_app_src_alloc(&fixture->app_src, TEST_PAYLOAD, K_NO_WAIT, &bufs[i]);
		zassert_ok(ret, "buffer %u was not released by the start", i);
		net_buf_unref(bufs[i]);
	}
}

/* Stopping with buffers pending releases them and posts no EOS */
ZTEST_F(test_app, test_stop_with_pending)
{
	struct mpipe_element *pipe = (struct mpipe_element *)&fixture->pipeline;
	const struct zbus_channel *chan;
	struct mpipe_message msg;
	struct net_buf *bufs[CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM];
	uint8_t payload[TEST_PAYLOAD] = {0};
	int ret;

	set_caps(&fixture->app_src, &fixture->app_sink);
	zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_PAUSED));

	/* Queued but never consumed: the pipeline thread is not running */
	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SRC_QUEUE_DEPTH; i++) {
		ret = mpipe_app_src_push(&fixture->app_src, payload, sizeof(payload), K_NO_WAIT);
		zassert_ok(ret, "push %u failed", i);
	}

	zassert_ok(mpipe_element_set_state(pipe, MPIPE_STATE_READY), "stop did not complete");
	zassert_equal(zbus_sub_wait_msg(&test_app_sub, &chan, &msg, K_MSEC(50)), -ENOMSG,
		      "a stop without EOS posted a message");

	for (uint8_t i = 0; i < CONFIG_MPIPE_BASE_APP_SRC_POOL_NUM; i++) {
		ret = mpipe_app_src_alloc(&fixture->app_src, TEST_PAYLOAD, K_NO_WAIT, &bufs[i]);
		zassert_ok(ret, "buffer %u was not released by the stop", i);
		net_buf_unref(bufs[i]);
	}
}
