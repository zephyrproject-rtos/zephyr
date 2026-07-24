/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/zbase/mp_queue.h>

LOG_MODULE_REGISTER(mp_queue, CONFIG_MP_LOG_LEVEL);

/*
 * Static EOS sentinel enqueued into the buffer queue to signal end-of-stream.
 * The queue thread recognizes this pointer and propagates EOS downstream only
 * after all preceding buffers have been processed.
 */
static uint8_t eos_sentinel;

/*
 * Static pause sentinel enqueued into the buffer queue to unblock the thread
 * from k_msgq_get() when transitioning to paused. The thread does not process
 * this value — it simply exits from k_msgq_get() to return to mp_thread_wait().
 */
static uint8_t pause_sentinel;

static int mp_queue_get_property(struct mp_object *obj, uint32_t id, void *val)
{
	struct mp_queue *queue = (struct mp_queue *)obj;

	switch (id) {
	case PROP_QUEUE_SIZE:
		*(uint8_t *)val = queue->size;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_queue_set_property(struct mp_object *obj, uint32_t id, const void *val)
{
	struct mp_queue *queue = (struct mp_queue *)obj;

	switch (id) {
	case PROP_QUEUE_SIZE:
		queue->size = *(const uint8_t *)val;
		if (!IN_RANGE(queue->size, 1, CONFIG_MP_QUEUE_MAX_SIZE)) {
			LOG_WRN("Requested size %u is out of range [1 %u]", queue->size,
				CONFIG_MP_QUEUE_MAX_SIZE);
			queue->size = CONFIG_MP_QUEUE_MAX_SIZE;
			return -EINVAL;
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_queue_chainfn(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out_buf)
{
	struct mp_queue *queue = (struct mp_queue *)pad->object.container;
	int ret;

	ret = k_msgq_put(&queue->msgq, &in_buf, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to put buffer into the buffer queue (%d)", ret);

		return ret;
	}

	*out_buf = NULL;

	return 0;
}

static int mp_queue_sink_eventfn(struct mp_pad *pad, struct mp_dispatch *event)
{
	struct mp_queue *queue = (struct mp_queue *)pad->object.container;
	int ret;

	switch (event->type) {
	case MP_DISPATCH_EOS:
		void *eos_ptr = &eos_sentinel;

		ret = k_msgq_put(&queue->msgq, &eos_ptr, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to put EOS sentinel to the msgq (%d)", ret);
		}

		return ret;
	case MP_DISPATCH_CAPS:
		struct mp_caps *caps = mp_dispatch_get_caps(event);

		if (caps == NULL || mp_caps_is_empty(caps)) {
			return -EINVAL;
		}
		queue->transform.set_caps(&queue->transform, MP_PAD_SINK, caps);
		queue->transform.set_caps(&queue->transform, MP_PAD_SRC, caps);
		mp_caps_unref(caps);

		return mp_pad_send_event(queue->transform.srcpad.peer, event);
	default:
		return -ENOTSUP;
	}
}

static void mp_queue_thread_func(void *p1, void *p2, void *p3)
{
	struct mp_queue *queue = p1;
	struct net_buf *buffer;
	int ret;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (mp_thread_wait(&queue->thread) == 0) {
		ret = k_msgq_get(&queue->msgq, &buffer, K_FOREVER);
		if (ret != 0) {
			LOG_DBG("Failed to get buffer from queue (%d)", ret);
			break;
		}

		if (buffer == (void *)&pause_sentinel) {
			LOG_DBG("Pause sentinel dequeued");
			continue;
		}

		if (buffer == (void *)&eos_sentinel) {
			struct mp_dispatch eos;

			LOG_DBG("EOS sentinel dequeued, sending EOS downstream");
			mp_dispatch_eos_init(&eos);
			ret = mp_pad_send_event(queue->transform.srcpad.peer, &eos);
			if (ret != 0) {
				LOG_ERR("Failed to send EOS event downstream (%d)", ret);
			}
			continue;
		}

		mp_pipeline_push_buffer(&queue->transform.srcpad, buffer);
	}

	LOG_DBG("Queue thread exiting");
}

static enum mp_state_change_return mp_queue_change_state(struct mp_element *element,
							 enum mp_state_change transition)
{
	struct mp_queue *queue = (struct mp_queue *)element;
	void *pause_ptr = &pause_sentinel;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		if (mp_thread_create(&queue->thread, mp_queue_thread_func, queue, NULL, NULL,
				     CONFIG_MP_THREAD_DEFAULT_PRIORITY, K_FOREVER) == NULL) {
			LOG_ERR("Failed to create a new queue thread");
			return MP_STATE_CHANGE_FAILURE;
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		mp_thread_resume(&queue->thread);
		break;
	case MP_STATE_CHANGE_PLAYING_TO_PAUSED:
		/*
		 * Mark the thread as paused, then inject a pause sentinel to
		 * unblock k_msgq_get(). The thread will see the sentinel and
		 * will continue the loop to block in wait().
		 */
		mp_thread_pause(&queue->thread);
		k_msgq_put(&queue->msgq, &pause_ptr, K_NO_WAIT);
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		struct net_buf *buffer;

		mp_thread_join(&queue->thread, K_FOREVER);
		/* Drain any remaining buffers from the message queue */
		LOG_DBG("Draining remaining buffers");
		while (k_msgq_get(&queue->msgq, &buffer, K_NO_WAIT) == 0) {
			if (buffer != (void *)&eos_sentinel && buffer != (void *)&pause_sentinel) {
				net_buf_unref(buffer);
			}
		}
		break;
	default:
		break;
	}

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_queue_init(struct mp_element *self)
{
	struct mp_queue *queue = (struct mp_queue *)self;

	mp_transform_init(self);

	self->object.set_property = mp_queue_set_property;
	self->object.get_property = mp_queue_get_property;
	self->change_state = mp_queue_change_state;

	queue->transform.sinkpad.chainfn = mp_queue_chainfn;
	queue->transform.sinkpad.eventfn = mp_queue_sink_eventfn;
	queue->size = CONFIG_MP_QUEUE_MAX_SIZE;

	/* Size of the msgq = queue's max size + 2 (for eos and pause sentinels) */
	k_msgq_init(&queue->msgq, queue->msgq_buffer, sizeof(void *), CONFIG_MP_QUEUE_MAX_SIZE + 2);
}
