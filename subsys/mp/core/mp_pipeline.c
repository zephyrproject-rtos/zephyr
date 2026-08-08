/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_bus.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>
#include <zephyr/mp/core/mp_pipeline.h>
#include <zephyr/mp/core/mp_src.h>

LOG_MODULE_REGISTER(mp_pipeline, CONFIG_MP_LOG_LEVEL);

int mp_pipeline_push_buffer(struct mp_pad *srcpad, struct net_buf *buffer)
{
	struct mp_pad *cur_srcpad = srcpad;
	struct mp_pad *next_sinkpad;
	struct mp_element *next_elem;
	struct net_buf *out_buf;
	sys_dnode_t *srcpad_node;
	int ret;

	if (cur_srcpad == NULL || buffer == NULL) {
		return -EINVAL;
	}

	while (cur_srcpad != NULL && buffer != NULL) {
		next_sinkpad = cur_srcpad->peer;

		if (next_sinkpad == NULL) {
			LOG_ERR("srcpad has no peer");
			net_buf_unref(buffer);
			return -ENOTCONN;
		}

		if (next_sinkpad->chainfn != NULL) {
			out_buf = NULL;

			ret = next_sinkpad->chainfn(next_sinkpad, buffer, &out_buf);
			if (ret != 0) {
				LOG_ERR("chainfn failed for element %u (%d)",
					next_sinkpad->object.container->id, ret);
				return ret;
			}

			if (out_buf == NULL) {
				/* Buffer consumed (by a sink or a queue element), exit the loop */
				break;
			}

			buffer = out_buf;
		}

		/* Move to the next element's first srcpad */
		next_elem = (struct mp_element *)next_sinkpad->object.container;
		srcpad_node = sys_dlist_peek_head(&next_elem->srcpads);
		if (srcpad_node == NULL) {
			/* Sink element reached - done */
			break;
		}
		cur_srcpad = CONTAINER_OF(srcpad_node, struct mp_pad, object.node);
	}

	return 0;
}

static void mp_pipeline_thread_func(void *p1, void *p2, void *p3)
{
	struct mp_bin *bin = p1;
	struct mp_pipeline *pipeline = p1;
	struct mp_object *obj;
	struct mp_element *element;
	struct mp_src *src = NULL;
	struct net_buf *buffer = NULL;
	struct mp_dispatch eos_event;
	uint32_t count = 0;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Find the 1st source element */
	SYS_DLIST_FOR_EACH_CONTAINER(&bin->children, obj, node) {
		element = (struct mp_element *)obj;
		if (sys_dlist_is_empty(&element->sinkpads)) {
			src = (struct mp_src *)element;
			break;
		}
	}

	if (src == NULL || src->pool == NULL || src->pool->acquire_buffer == NULL) {
		return;
	}

	while (mp_thread_wait(&pipeline->thread) == 0) {
		if ((src->num_buffers != 0 && count == src->num_buffers) ||
		    src->pool->acquire_buffer(src->pool, &buffer) != 0) {
			mp_dispatch_eos_init(&eos_event);
			if (mp_pad_send_event(src->srcpad.peer, &eos_event) != 0) {
				LOG_ERR("Failed to send EOS event downstream");
			}
			count = 0;
			/* Self-pause: block in wait() until next resume */
			mp_thread_pause(&pipeline->thread);
			continue;
		}
		count++;
		if (mp_pipeline_push_buffer(&src->srcpad, buffer) != 0) {
			LOG_ERR("Failed to push buffer downstream");
		}
	}

	LOG_DBG("Pipeline thread exiting");
}

static enum mp_state_change_return mp_pipeline_change_state(struct mp_element *element,
							    enum mp_state_change transition)
{
	struct mp_pipeline *pipeline = (struct mp_pipeline *)element;
	enum mp_state_change_return ret;

	/*
	 * DOWN: Pipeline thread should be handled before children state change, i.e. source needs
	 * to stop producing buffers first
	 */
	switch (transition) {
	case MP_STATE_CHANGE_PLAYING_TO_PAUSED:
		mp_thread_pause(&pipeline->thread);
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		mp_thread_join(&pipeline->thread, K_FOREVER);
		break;
	default:
		break;
	}

	/* Children state change: UP = sink-to-source, DOWN=source-to-sink*/
	ret = mp_bin_change_state_func(element, transition);
	if (ret != MP_STATE_CHANGE_SUCCESS) {
		return ret;
	}

	/*
	 * UP: Pipeline thread should be handled after children state change, i.e., children need to
	 * be prepared before receiving buffers from source
	 */
	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		/* Create the thread but do not start it (K_FOREVER) */
		if (mp_thread_create(&pipeline->thread, mp_pipeline_thread_func, element, NULL,
				     NULL, CONFIG_MP_THREAD_DEFAULT_PRIORITY, K_FOREVER) == NULL) {
			LOG_ERR("Failed to create a new pipeline thread");
			return MP_STATE_CHANGE_FAILURE;
		}
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		mp_thread_resume(&pipeline->thread);
		break;
	default:
		break;
	}

	LOG_DBG("Pipeline id %u has changed state to %u", element->object.id,
		MP_STATE_TRANSITION_NEXT(transition));

	return MP_STATE_CHANGE_SUCCESS;
}

void mp_pipeline_init(struct mp_element *self)
{
	/* Initialize base class */
	mp_bin_init(self);
	self->change_state = mp_pipeline_change_state;
}
