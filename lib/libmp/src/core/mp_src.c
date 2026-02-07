/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_pad.h"
#include "mp_property.h"
#include "mp_src.h"
#include "mp_task.h"

LOG_MODULE_REGISTER(mp_src, CONFIG_LIBMP_LOG_LEVEL);

#define MP_PAD_SRC_ID 0

int mp_src_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_src *src = MP_SRC(obj);

	switch (key) {
	case PROP_NUM_BUFS:
		src->num_buffers = (uint32_t)(uintptr_t)val;
		return 0;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}
}

int mp_src_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_src *src = MP_SRC(obj);

	switch (key) {
	case PROP_NUM_BUFS:
		*(uint32_t *)val = src->num_buffers;
		break;
	default:
		LOG_ERR("Property %d is unknown", key);
		return -ENOTSUP;
	}

	return 0;
}

static bool mp_src_query(struct mp_pad *pad, struct mp_query *query)
{
	bool ret = false;
	struct mp_src *src = MP_SRC(pad->object.container);
	struct mp_caps *intersect_caps, *query_caps;

	switch (query->type) {
	case MP_QUERY_CAPS:
		query_caps = mp_query_get_caps(query);
		if (query_caps != NULL) {
			intersect_caps = mp_caps_intersect(src->srcpad.caps, query_caps);
			ret = mp_query_set_caps(query, intersect_caps);
			mp_caps_unref(intersect_caps);
		} else {
			ret = mp_query_set_caps(query, src->srcpad.caps);
		}
		break;
	default:
		ret = mp_pad_query(pad->peer, query);
		break;
	}

	return ret;
}

static bool mp_src_decide_allocation(struct mp_src *self, struct mp_query *query)
{
	return true;
}

static bool mp_src_negotiate(struct mp_src *src)
{
	bool ret = false;
	struct mp_caps *fixated_caps;
	struct mp_query *caps_query, *alloc_query;
	struct mp_event *caps_event;

	/* Caps negotiation */
	if (src->srcpad.caps == NULL) {
		return false;
	}

	/* Query the peer's capabilities */
	caps_query = mp_query_new_caps(src->srcpad.caps);
	if (!mp_pad_query(src->srcpad.peer, caps_query)) {
		mp_query_destroy(caps_query);
		return false;
	}

	/* Fixate the common caps */
	fixated_caps = mp_caps_fixate(mp_query_get_caps(caps_query));
	mp_query_destroy(caps_query);
	if (fixated_caps == NULL) {
		return false;
	}

	/* Send caps event to configure downstream */
	caps_event = mp_event_new_caps(fixated_caps);
	ret = mp_pad_send_event(src->srcpad.peer, caps_event);
	if (ret && !src->set_caps(src, fixated_caps)) {
		mp_caps_unref(fixated_caps);
		mp_event_destroy(caps_event);
		return false;
	}
	mp_caps_unref(fixated_caps);
	mp_event_destroy(caps_event);

	/* Query the peer's allocation proposal */
	alloc_query = mp_query_new_allocation(src->srcpad.caps);
	if (!mp_pad_query(src->srcpad.peer, alloc_query)) {
		mp_query_destroy(alloc_query);
		return false;
	}

	/* Decide the allocation */
	ret = src->decide_allocation(src, alloc_query);
	mp_query_destroy(alloc_query);

	return ret;
}

static void mp_src_loop(void *pad, void *userdata)
{
	struct mp_src *src = MP_SRC(MP_PAD(pad)->object.container);
	struct mp_buffer *buffer = NULL;
	struct mp_message *eos_message = NULL;
	uint32_t count = 0;

	MP_PAD(pad)->task.running = true;
	while (MP_PAD(pad)->task.running && count++ <= src->num_buffers) {
		if (MP_OBJECT(pad)->flags & MP_PAD_FLAG_NEGOTIATE) {
			if (!mp_src_negotiate(src)) {
				LOG_ERR("Negotiation failed");
				break;
			}

			/* Config buffer pool */
			if (!MP_PAD(pad)->caps) {
				LOG_ERR("No source pad capabilities configured");
				break;
			}

			if (!src->pool->configure(src->pool,
						  mp_caps_get_structure(MP_PAD(pad)->caps, 0))) {
				LOG_ERR("Failed to configure buffer pool");
				break;
			}

			/* Start buffer pool */
			src->pool->start(src->pool);

			/* Clear MP_PAD_FLAG_NEGOTIATE flag */
			MP_OBJECT(pad)->flags &= ~MP_PAD_FLAG_NEGOTIATE;
		}

		src->pool->acquire_buffer(src->pool, &buffer);
		mp_pad_push(pad, buffer);
	}

	if (count > src->num_buffers) {
		/* Stop the thread and post EOS message to the pipeline's bus */
		MP_PAD(pad)->task.running = false;
		eos_message = mp_message_new(MP_MESSAGE_EOS, MP_OBJECT(src), NULL);
		mp_bus_post(mp_element_get_bus(MP_ELEMENT(src)), eos_message);
	}
}

static enum mp_state_change_return mp_src_change_state(struct mp_element *self,
						       enum mp_state_change transition)
{
	enum mp_state_change_return ret = MP_STATE_CHANGE_SUCCESS;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		/* TODO */
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		mp_pad_start_task(&MP_SRC(self)->srcpad, (mp_task_function)mp_src_loop,
				  CONFIG_MP_THREAD_DEFAULT_PRIORITY, NULL);
		break;
	default:
		break;
	}

	return ret;
}

static bool mp_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	return true;
}

void mp_src_init(struct mp_element *self)
{
	struct mp_src *src = MP_SRC(self);

	/* Add pad */
	mp_pad_init(&src->srcpad, MP_PAD_SRC_ID, MP_PAD_SRC, MP_PAD_ALWAYS, NULL);
	mp_element_add_pad(self, &src->srcpad);

	/* Initialize property values */
	src->num_buffers = UINT32_MAX;

	self->object.set_property = mp_src_set_property;
	self->object.get_property = mp_src_get_property;
	self->change_state = mp_src_change_state;

	src->set_caps = mp_src_set_caps;
	src->srcpad.queryfn = mp_src_query;
	src->decide_allocation = mp_src_decide_allocation;

	/* Set MP_PAD_FLAG_NEGOTIATE flag */
	MP_OBJECT(&src->srcpad)->flags |= MP_PAD_FLAG_NEGOTIATE;
}
