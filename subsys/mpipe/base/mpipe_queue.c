/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/base/mpipe_queue.h>

LOG_MODULE_REGISTER(mpipe_queue, CONFIG_MPIPE_LOG_LEVEL);

/*
 * Static EOS sentinel enqueued into the buffer queue to signal end-of-stream.
 * The queue thread recognizes this pointer and propagates EOS downstream only
 * after all preceding buffers have been processed.
 */
static uint8_t eos_sentinel;

/*
 * Static pause sentinel enqueued into the buffer queue to unblock the thread
 * from k_msgq_get() when transitioning to paused. The thread does not process
 * this value - it simply exits from k_msgq_get() to return to mpipe_thread_wait().
 */
static uint8_t pause_sentinel;

static int mpipe_queue_get_property(struct mpipe_object *obj, uint32_t id, void *val)
{
	struct mpipe_queue *queue = (struct mpipe_queue *)obj;

	switch (id) {
	case MPIPE_PROP_BASE_QUEUE_SIZE:
		*(uint8_t *)val = queue->size;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_queue_set_property(struct mpipe_object *obj, uint32_t id, const void *val)
{
	struct mpipe_queue *queue = (struct mpipe_queue *)obj;

	switch (id) {
	case MPIPE_PROP_BASE_QUEUE_SIZE:
		queue->size = *(const uint8_t *)val;
		if (!IN_RANGE(queue->size, 1, CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE)) {
			LOG_WRN("Requested size %u is out of range [1 %u]", queue->size,
				CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE);
			queue->size = CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE;
			return -EINVAL;
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_queue_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				struct net_buf **out_buf)
{
	struct mpipe_queue *queue = (struct mpipe_queue *)pad->object.container;
	int ret;

	/*
	 * If the queue is flushing (teardown to READY), drop the buffer instead
	 * of enqueuing it. This keeps a producer that was just released from a
	 * blocking k_msgq_put() from re-blocking, and prevents a late buffer from
	 * leaking into an already-drained queue (e.g. behind a tee).
	 */
	if (atomic_get(&queue->flushing) != 0) {
		net_buf_unref(in_buf);
		*out_buf = NULL;
		return 0;
	}

	ret = k_msgq_put(&queue->msgq, &in_buf, K_FOREVER);
	if (ret != 0) {
		/*
		 * A non-zero return here means the put was interrupted (e.g. the
		 * queue was purged/started flushing). Drop the buffer and report
		 * success so the release path unwinds cleanly without error spam.
		 */
		net_buf_unref(in_buf);
		*out_buf = NULL;
		return 0;
	}

	*out_buf = NULL;

	return 0;
}

static int mpipe_queue_sink_event_fn(struct mpipe_pad *pad, struct mpipe_dispatch *event)
{
	struct mpipe_queue *queue = (struct mpipe_queue *)pad->object.container;
	int ret;

	switch (event->type) {
	case MPIPE_DISPATCH_EOS:
		void *eos_ptr = &eos_sentinel;

		/* Drop EOS if flushing (teardown); nothing downstream needs it. */
		if (atomic_get(&queue->flushing) != 0) {
			return 0;
		}

		ret = k_msgq_put(&queue->msgq, &eos_ptr, K_FOREVER);
		if (ret != 0) {
			/* Interrupted by a flush; treat as consumed. */
			return 0;
		}

		return ret;
	case MPIPE_DISPATCH_CAPS:
		struct mpipe_structure *caps = event->caps;

		/* An event carrying no capability is informational: just forward */
		if (caps == NULL) {
			return mpipe_pad_send_event(queue->transform.src_pad.peer, event);
		}

		if (mpipe_structure_is_empty(caps)) {
			return -EINVAL;
		}
		queue->transform.set_caps(&queue->transform, MPIPE_PAD_SINK, caps);
		queue->transform.set_caps(&queue->transform, MPIPE_PAD_SRC, caps);

		return mpipe_pad_send_event(queue->transform.src_pad.peer, event);
	default:
		return -ENOTSUP;
	}
}

static void mpipe_queue_thread_func(void *p1, void *p2, void *p3)
{
	struct mpipe_queue *queue = p1;
	struct net_buf *buffer;
	int ret;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (mpipe_thread_wait(&queue->thread) == 0) {
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
			struct mpipe_dispatch eos = {.type = MPIPE_DISPATCH_EOS};

			LOG_DBG("EOS sentinel dequeued, sending EOS downstream");
			ret = mpipe_pad_send_event(queue->transform.src_pad.peer, &eos);
			if (ret != 0) {
				struct mpipe_message msg = {
					.origin = &queue->transform.element,
					.type = MPIPE_MESSAGE_ERROR,
					.domain = MPIPE_ERROR_FLOW,
					.code = ret,
				};

				LOG_ERR("Failed to send EOS event downstream (%d)", ret);

				/*
				 * No sink downstream of this queue will post the
				 * end of stream now, and the application is
				 * waiting for that or an error.
				 */
				(void)mpipe_message_post(&msg);
			}
			continue;
		}

		mpipe_push_buffer(&queue->transform.src_pad, buffer);
	}

	LOG_DBG("Queue thread exiting");
}

static enum mpipe_state_change_return mpipe_queue_change_state(struct mpipe_element *element,
							       enum mpipe_state_change transition)
{
	struct mpipe_queue *queue = (struct mpipe_queue *)element;
	void *pause_ptr = &pause_sentinel;

	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		/* Not flushing while active: accept incoming buffers. */
		atomic_set(&queue->flushing, 0);
		if (mpipe_thread_create(&queue->thread, mpipe_queue_thread_func, queue, NULL, NULL,
					CONFIG_MPIPE_THREAD_DEFAULT_PRIORITY, K_FOREVER) == NULL) {
			LOG_ERR("Failed to create a new queue thread");
			return MPIPE_STATE_CHANGE_FAILURE;
		}
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_PLAYING:
		atomic_set(&queue->flushing, 0);
		mpipe_thread_resume(&queue->thread);
		break;
	case MPIPE_STATE_CHANGE_PLAYING_TO_PAUSED:
		/*
		 * Mark the thread as paused, then inject a pause sentinel to
		 * unblock k_msgq_get(). The thread will see the sentinel and
		 * will continue the loop to block in wait().
		 */
		mpipe_thread_pause(&queue->thread);
		k_msgq_put(&queue->msgq, &pause_ptr, K_NO_WAIT);
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		struct net_buf *buffer;

		/*
		 * Enter flushing before joining. Any producer blocked in this
		 * queue's k_msgq_put() is released once the drain below frees a
		 * slot; the flushing flag then makes its (and any subsequent)
		 * chain_fn drop the buffer instead of re-enqueuing or leaking it.
		 */
		atomic_set(&queue->flushing, 1);
		mpipe_thread_join(&queue->thread, K_FOREVER);

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

	/*
	 * Chain to the base transform change_state. Among other things it resets
	 * the negotiated pad caps back to the template caps on PAUSED_TO_READY so
	 * a subsequent re-negotiation starts fresh.
	 */
	return mpipe_transform_change_state(element, transition);
}

int mpipe_queue_init(struct mpipe_queue *queue, uint8_t id)
{
	__ASSERT_NO_MSG(queue != NULL);

	struct mpipe_element *self = &queue->transform.element;
	int ret = mpipe_transform_init(&queue->transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "queue");

	self->object.set_property = mpipe_queue_set_property;
	self->object.get_property = mpipe_queue_get_property;
	self->change_state = mpipe_queue_change_state;

	queue->transform.sink_pad.chain_fn = mpipe_queue_chain_fn;
	queue->transform.sink_pad.event_fn = mpipe_queue_sink_event_fn;
	queue->size = CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE;

	/* Size of the msgq = queue's max size + 2 (for eos and pause sentinels) */
	k_msgq_init(&queue->msgq, queue->msgq_buffer, sizeof(void *),
		    CONFIG_MPIPE_BASE_QUEUE_MAX_SIZE + 2);

	return 0;
}
