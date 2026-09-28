/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/mpipe_object.h>
#include <zephyr/mpipe/mpipe_pad.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_src.h>

LOG_MODULE_REGISTER(mpipe_pipeline, CONFIG_MPIPE_LOG_LEVEL);

static int mpipe_pipeline_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe *pipeline = (struct mpipe *)obj;

	switch (key) {
	case MPIPE_PROP_PIPELINE_THREAD_PRIORITY:
		pipeline->thread.priority = *(const int *)val;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

static int mpipe_pipeline_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe *pipeline = (struct mpipe *)obj;

	switch (key) {
	case MPIPE_PROP_PIPELINE_THREAD_PRIORITY:
		*(int *)val = pipeline->thread.priority;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

/* Each sink posts an EOS; only the last one of a run reaches the application */
static inline bool mpipe_pipeline_validate_eos(struct mpipe *p)
{
	uint32_t seen = (uint32_t)atomic_inc(&p->eos_count) + 1;

	return p->num_sinks == 0 || seen >= p->num_sinks;
}

/* Bus validator, run in the posting thread before the message is delivered */
static bool mpipe_pipeline_message_validator(const void *msg, size_t msg_size)
{
	const struct mpipe_message *m = msg;
	struct mpipe *pipeline;
	struct zbus_channel *chan;

	ARG_UNUSED(msg_size);

	if (m->origin == NULL) {
		return true;
	}

	chan = mpipe_element_get_bus_chan(m->origin);
	if (chan == NULL) {
		return true;
	}

	pipeline = zbus_chan_user_data(chan);
	if (pipeline == NULL) {
		return true;
	}

	switch (m->type) {
	case MPIPE_MESSAGE_EOS:
		return mpipe_pipeline_validate_eos(pipeline);
	default:
		/* Pass-through for all other type of messages */
		return true;
	}
}

static uint32_t mpipe_pipeline_count_sinks(struct mpipe_bin *bin)
{
	struct mpipe_object *obj;
	struct mpipe_element *element;
	uint32_t count = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = (struct mpipe_element *)obj;
		if (sys_dlist_is_empty(&element->src_pads)) {
			count++;
		}
	}

	return count;
}

static void mpipe_pipeline_set_flushing(struct mpipe_bin *bin, bool flush)
{
	struct mpipe_object *obj;
	struct mpipe_element *element;
	struct mpipe_object *pad_obj;

	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = (struct mpipe_element *)obj;

		SYS_DLIST_FOR_EACH_CONTAINER(&element->sink_pads, pad_obj, node) {
			atomic_set(&((struct mpipe_pad *)pad_obj)->flushing, flush ? 1 : 0);
		}

		SYS_DLIST_FOR_EACH_CONTAINER(&element->src_pads, pad_obj, node) {
			atomic_set(&((struct mpipe_pad *)pad_obj)->flushing, flush ? 1 : 0);
		}
	}
}

int mpipe_push_buffer(struct mpipe_pad *src_pad, struct net_buf *buffer)
{
	struct mpipe_pad *cur_src_pad = src_pad;
	struct mpipe_pad *next_sink_pad;
	struct mpipe_element *next_elem;
	struct net_buf *out_buf;
	sys_dnode_t *src_pad_node;
	int ret;

	if (cur_src_pad == NULL || buffer == NULL) {
		return -EINVAL;
	}

	while (cur_src_pad != NULL && buffer != NULL) {
		next_sink_pad = cur_src_pad->peer;

		if (next_sink_pad == NULL) {
			struct mpipe_message msg = {
				.origin = (struct mpipe_element *)cur_src_pad->object.container,
				.type = MPIPE_MESSAGE_ERROR,
				.domain = MPIPE_ERROR_FLOW,
				.code = -ENOTCONN,
			};

			LOG_ERR("src_pad has no peer");
			net_buf_unref(buffer);
			(void)mpipe_message_post(&msg);
			return -ENOTCONN;
		}

		/* Flushing: the element downstream is being torn down */
		if (atomic_get(&next_sink_pad->flushing) != 0) {
			net_buf_unref(buffer);
			return 0;
		}

		if (next_sink_pad->chain_fn != NULL) {
			out_buf = NULL;

			ret = next_sink_pad->chain_fn(next_sink_pad, buffer, &out_buf);
			if (ret != 0) {
				struct mpipe_element *elem =
					(struct mpipe_element *)next_sink_pad->object.container;
				struct mpipe_message msg = {
					.origin = elem,
					.type = MPIPE_MESSAGE_ERROR,
					.domain = MPIPE_ERROR_FLOW,
					.code = ret,
				};

				LOG_ERR("chain_fn failed for element %u (%d)",
					next_sink_pad->object.container->id, ret);

				/* No caller to return this to: the bus is the only way out */
				(void)mpipe_message_post(&msg);
				return ret;
			}

			if (out_buf == NULL) {
				/* Buffer consumed (by a sink or a queue element), exit the loop */
				break;
			}

			buffer = out_buf;
		}

		/* Move to the next element's first src_pad */
		next_elem = (struct mpipe_element *)next_sink_pad->object.container;
		src_pad_node = sys_dlist_peek_head(&next_elem->src_pads);
		if (src_pad_node == NULL) {
			/* Sink element reached - done */
			break;
		}
		cur_src_pad = CONTAINER_OF(src_pad_node, struct mpipe_pad, object.node);
	}

	return 0;
}

/* No sink will post EOS after a failed send, so the failure has to arrive as an error */
static void mpipe_pipeline_send_eos(struct mpipe_src *src)
{
	struct mpipe_dispatch eos_event = {.type = MPIPE_DISPATCH_EOS};
	int ret;

	ret = mpipe_pad_send_event(src->src_pad.peer, &eos_event);
	if (ret != 0) {
		struct mpipe_message msg = {
			.origin = &src->element,
			.type = MPIPE_MESSAGE_ERROR,
			.domain = MPIPE_ERROR_FLOW,
			.code = ret,
		};

		LOG_ERR("Failed to send EOS event downstream (%d)", ret);
		(void)mpipe_message_post(&msg);
	}
}

static void mpipe_pipeline_thread_func(void *p1, void *p2, void *p3)
{
	struct mpipe_bin *bin = p1;
	struct mpipe *pipeline = p1;
	struct mpipe_object *obj;
	struct mpipe_element *element;
	struct mpipe_src *src = NULL;
	struct net_buf *buffer = NULL;
	uint32_t count = 0;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Find the 1st source element */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = (struct mpipe_element *)obj;
		if (sys_dlist_is_empty(&element->sink_pads)) {
			src = (struct mpipe_src *)element;
			break;
		}
	}

	if (src == NULL || src->pool == NULL || src->pool->acquire_buffer == NULL) {
		return;
	}

	while (mpipe_thread_wait(&pipeline->thread) == 0) {
		int acq_ret = 0;
		bool reached_limit = (src->num_buffers != 0 && count == src->num_buffers);

		if (!reached_limit) {
			acq_ret = src->pool->acquire_buffer(src->pool, &buffer);
		}

		/* EOS: a playback source ran out of data, or a live source hit its limit */
		bool is_eos = reached_limit || acq_ret == -ENODATA;

		if (reached_limit || acq_ret != 0) {
			if (is_eos) {
				mpipe_pipeline_send_eos(src);
			} else if (acq_ret != -EPIPE) {
				struct mpipe_message msg = {
					.origin = &src->element,
					.type = MPIPE_MESSAGE_ERROR,
					.domain = MPIPE_ERROR_FLOW,
					.code = acq_ret,
				};

				/* Neither EOS nor a forced-stop flush (-EPIPE): a real error */
				LOG_ERR("Source failed to acquire a buffer (%d)", acq_ret);
				(void)mpipe_message_post(&msg);
			}
			count = 0;
			/* Self-pause: block in wait() until next resume */
			mpipe_thread_pause(&pipeline->thread);
			continue;
		}
		count++;
		if (mpipe_push_buffer(&src->src_pad, buffer) != 0) {
			LOG_ERR("Failed to push buffer downstream");
			/* Fatal to the stream: stop producing so one error, not a flood */
			count = 0;
			mpipe_thread_pause(&pipeline->thread);
		}
	}

	LOG_DBG("Pipeline thread exiting");
}

static int mpipe_pipeline_change_state(struct mpipe_element *element,
				       enum mpipe_state_change transition)
{
	struct mpipe *pipeline = (struct mpipe *)element;
	int ret;

	/* Going down, the source stops producing before the children change state */
	switch (transition) {
	case MPIPE_STATE_CHANGE_PLAYING_TO_PAUSED:
		/* A pause keeps the queued buffers: nothing is flushed or joined */
		mpipe_thread_pause(&pipeline->thread);
		break;
	case MPIPE_STATE_CHANGE_PAUSED_TO_READY:
		/* Raise the flushing gate before the children tear down their caps and pools */
		mpipe_pipeline_set_flushing(&pipeline->bin, true);
		break;
	default:
		break;
	}

	/* Children state change: UP = sink-to-source, DOWN = source-to-sink */
	ret = mpipe_bin_change_state_func(element, transition);

	if (ret != 0) {
		return ret;
	}

	/* Join after the children have drained, which frees a thread blocked in a full queue */
	if (transition == MPIPE_STATE_CHANGE_PAUSED_TO_READY) {
		mpipe_thread_join(&pipeline->thread, K_FOREVER);
		/* Reset EOS counter for a clean re-run. */
		atomic_set(&pipeline->eos_count, 0);
	}

	/* Going up, the children are ready before the thread produces anything */
	switch (transition) {
	case MPIPE_STATE_CHANGE_READY_TO_PAUSED:
		/* Clear the flushing gate so buffers can flow again */
		mpipe_pipeline_set_flushing(&pipeline->bin, false);

		/* Create the thread but do not start it (K_FOREVER) */
		if (mpipe_thread_create(&pipeline->thread, mpipe_pipeline_thread_func, element,
					NULL, NULL, pipeline->thread.priority, K_FOREVER) == NULL) {
			LOG_ERR("Failed to create a new pipeline thread");
			return -EAGAIN;
		}

		/* Arm the EOS folding before the thread can produce one */
		pipeline->num_sinks = mpipe_pipeline_count_sinks(&pipeline->bin);
		atomic_set(&pipeline->eos_count, 0);
		break;

	case MPIPE_STATE_CHANGE_PAUSED_TO_PLAYING:
		mpipe_thread_resume(&pipeline->thread);
		break;
	default:
		break;
	}

	LOG_DBG("Pipeline id %u has changed state to %u", element->object.id,
		MPIPE_STATE_TRANSITION_NEXT(transition));

	return 0;
}

int mpipe_pipeline_init(struct mpipe *pipe, uint8_t id)
{
	__ASSERT_NO_MSG(pipe != NULL);

	struct mpipe_element *self = &pipe->bin.element;
	int ret = mpipe_bin_init(&pipe->bin, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "pipeline");

	/* Initialize base class */
	self->change_state = mpipe_pipeline_change_state;
	self->object.set_property = mpipe_pipeline_set_property;
	self->object.get_property = mpipe_pipeline_get_property;

	/* Default thread priority; caller may override before the first play. */
	pipe->thread.priority = CONFIG_MPIPE_THREAD_DEFAULT_PRIORITY;

	/* Only the pipeline knows how many sinks a run must hear from */
	return mpipe_bin_set_bus_validator(&pipe->bin, mpipe_pipeline_message_validator, pipe);
}
