/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/base/mpipe_queue.h>

#define QUEUE_ID 1
#define BUFS_NUM 3
#define BUF_SIZE 16

NET_BUF_POOL_FIXED_DEFINE(test_pool, BUFS_NUM, BUF_SIZE, 0, NULL);

struct test_queue_fixture {
	struct mpipe_queue queue;
	struct net_buf *bufs[BUFS_NUM];
};

static void *queue_suite_setup(void)
{
	static struct test_queue_fixture fixture;

	return &fixture;
}

static void queue_before(void *f)
{
	struct test_queue_fixture *fix = f;

	memset(fix, 0, sizeof(*fix));
	zassert_ok(mpipe_queue_init(&fix->queue, QUEUE_ID));

	for (uint8_t i = 0; i < BUFS_NUM; i++) {
		fix->bufs[i] = net_buf_alloc(&test_pool, K_NO_WAIT);
		zassert_not_null(fix->bufs[i], "buffer %u", i);
	}
}

static void queue_after(void *f)
{
	struct test_queue_fixture *fix = f;

	/* Whatever a failed test left in flight goes back to the pool */
	for (uint8_t i = 0; i < BUFS_NUM; i++) {
		if (fix->bufs[i] != NULL && fix->bufs[i]->ref > 0) {
			net_buf_unref(fix->bufs[i]);
		}
	}
}

ZTEST_SUITE(test_queue, NULL, queue_suite_setup, queue_before, queue_after, NULL);

static struct mpipe_element *queue_element(struct mpipe_queue *queue)
{
	return &queue->transform.element;
}

static struct mpipe_object *queue_object(struct mpipe_queue *queue)
{
	return &queue->transform.element.object;
}

/*
 * READY -> PAUSED creates the queue thread with an indefinite start delay,
 * so nothing consumes the msgq until PAUSED -> PLAYING: what the chain
 * function leaves in it can be inspected.
 */
static void queue_enter_paused(struct mpipe_queue *queue, uint8_t size, uint8_t leak)
{
	enum mpipe_base_queue_leak policy = leak;

	zassert_ok(mpipe_object_set_properties(queue_object(queue), MPIPE_PROP_BASE_QUEUE_SIZE,
					       &size, MPIPE_PROP_BASE_QUEUE_LEAK, &policy,
					       MPIPE_PROP_LIST_END));
	zassert_ok(queue_element(queue)->change_state(queue_element(queue),
						      MPIPE_STATE_CHANGE_READY_TO_PAUSED));
}

static void queue_leave_paused(struct mpipe_queue *queue)
{
	zassert_ok(queue_element(queue)->change_state(queue_element(queue),
						      MPIPE_STATE_CHANGE_PAUSED_TO_READY));
	zassert_equal(k_msgq_num_used_get(&queue->msgq), 0, "teardown left buffers queued");
}

static void queue_chain(struct mpipe_queue *queue, struct net_buf *buf)
{
	struct net_buf *out = NULL;
	struct mpipe_pad *sink_pad = &queue->transform.sink_pad;

	zassert_ok(sink_pad->chain_fn(sink_pad, buf, &out));
	zassert_is_null(out, "the queue hands nothing back synchronously");
}

static struct net_buf *queue_head(struct mpipe_queue *queue)
{
	struct net_buf *head = NULL;

	zassert_ok(k_msgq_peek(&queue->msgq, &head));

	return head;
}

ZTEST_F(test_queue, test_size_property_bounds_the_msgq)
{
	struct mpipe_queue *queue = &fixture->queue;
	uint8_t size = CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE + 1;
	uint8_t read_back = 0;

	zassert_equal(mpipe_object_set_properties(queue_object(queue), MPIPE_PROP_BASE_QUEUE_SIZE,
						  &size, MPIPE_PROP_LIST_END),
		      -EINVAL, "an out-of-range size is refused");

	queue_enter_paused(queue, 2, MPIPE_BASE_QUEUE_LEAK_NONE);

	zassert_ok(mpipe_object_get_properties(queue_object(queue), MPIPE_PROP_BASE_QUEUE_SIZE,
					       &read_back, MPIPE_PROP_LIST_END));
	zassert_equal(read_back, 2);
	zassert_equal(queue->msgq.max_msgs, 2 + 2, "size plus the two sentinel slots");

	queue_leave_paused(queue);
}

ZTEST_F(test_queue, test_leak_oldest_keeps_the_freshest)
{
	struct mpipe_queue *queue = &fixture->queue;

	queue_enter_paused(queue, 1, MPIPE_BASE_QUEUE_LEAK_OLDEST);

	for (uint8_t i = 0; i < BUFS_NUM; i++) {
		queue_chain(queue, fixture->bufs[i]);
	}

	zassert_equal(k_msgq_num_used_get(&queue->msgq), 1);
	zassert_equal_ptr(queue_head(queue), fixture->bufs[2]);
	zassert_equal(fixture->bufs[0]->ref, 0, "the oldest was released");
	zassert_equal(fixture->bufs[1]->ref, 0, "the next oldest was released");

	queue_leave_paused(queue);
}

ZTEST_F(test_queue, test_leak_newest_keeps_the_order)
{
	struct mpipe_queue *queue = &fixture->queue;

	queue_enter_paused(queue, 1, MPIPE_BASE_QUEUE_LEAK_NEWEST);

	for (uint8_t i = 0; i < BUFS_NUM; i++) {
		queue_chain(queue, fixture->bufs[i]);
	}

	zassert_equal(k_msgq_num_used_get(&queue->msgq), 1);
	zassert_equal_ptr(queue_head(queue), fixture->bufs[0]);
	zassert_equal(fixture->bufs[1]->ref, 0, "the arriving buffer was released");
	zassert_equal(fixture->bufs[2]->ref, 0, "the arriving buffer was released");

	queue_leave_paused(queue);
}

ZTEST_F(test_queue, test_leak_never_drops_eos)
{
	struct mpipe_queue *queue = &fixture->queue;
	struct mpipe_pad *sink_pad = &queue->transform.sink_pad;
	struct mpipe_dispatch eos = {.type = MPIPE_DISPATCH_EOS};

	queue_enter_paused(queue, 1, MPIPE_BASE_QUEUE_LEAK_OLDEST);

	queue_chain(queue, fixture->bufs[0]);
	zassert_ok(sink_pad->event_fn(sink_pad, &eos));
	zassert_equal(k_msgq_num_used_get(&queue->msgq), 2, "EOS is queued behind the buffer");

	/* A full queue makes room from a buffer, never from the sentinel */
	queue_chain(queue, fixture->bufs[1]);
	zassert_equal(fixture->bufs[0]->ref, 0, "the buffer ahead of EOS was released");
	zassert_equal(k_msgq_num_used_get(&queue->msgq), 2);

	queue_leave_paused(queue);
}
