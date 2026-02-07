/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "mp_element.h"
#include "mp_buffer.h"
#include "mp_pad.h"
#include "mp_task.h"

LOG_MODULE_REGISTER(mp_pad, CONFIG_LIBMP_LOG_LEVEL);

void mp_pad_init(struct mp_pad *pad, uint8_t id, enum mp_pad_direction direction,
		 enum mp_pad_presence presence, struct mp_caps *caps)
{
	__ASSERT_NO_MSG(pad != NULL);

	pad->object.id = id;
	pad->direction = direction;
	pad->presence = presence;
	pad->caps = caps;
	pad->eventfn = mp_pad_send_event_default;
}

struct mp_pad *mp_pad_new(uint8_t id, enum mp_pad_direction direction,
			  enum mp_pad_presence presence, struct mp_caps *caps)
{
	struct mp_pad *pad = k_malloc(sizeof(struct mp_pad));

	mp_pad_init(pad, id, direction, presence, caps);

	return pad;
}

bool mp_pad_link(struct mp_pad *srcpad, struct mp_pad *sinkpad)
{
	if (srcpad == NULL || sinkpad == NULL) {
		return false;
	}

	/* Set peer pad */
	srcpad->peer = sinkpad;
	sinkpad->peer = srcpad;

	if (srcpad->linkfn != NULL && !srcpad->linkfn(srcpad)) {
		return false;
	}

	if (sinkpad->linkfn != NULL && !sinkpad->linkfn(sinkpad)) {
		return false;
	}

	return true;
}

bool mp_pad_start_task(struct mp_pad *pad, mp_task_function func, int priority, void *user_data)
{
	k_tid_t thread = NULL;

	if (pad == NULL || pad->task.running) {
		return false;
	}

	thread = mp_task_create(&pad->task, func, user_data, priority);

	return thread != NULL;
}

bool mp_pad_chain(struct mp_pad *pad, struct mp_buffer *buffer)
{
	return pad->chainfn(pad, buffer);
}

bool mp_pad_push(struct mp_pad *pad, struct mp_buffer *buffer)
{
	return mp_pad_chain(pad->peer, buffer);
}

bool mp_pad_query(struct mp_pad *pad, struct mp_query *query)
{
	if (pad == NULL || query == NULL || pad->queryfn == NULL) {
		return false;
	}

	return pad->queryfn(pad, query);
}

bool mp_pad_send_event_default(struct mp_pad *pad, struct mp_event *event)
{
	bool ret = false;

	if (pad == NULL || event == NULL) {
		return false;
	}

	bool is_sink = (pad->direction == MP_PAD_SINK);
	bool is_src = (pad->direction == MP_PAD_SRC);
	bool is_upstream = (MP_EVENT_DIRECTION(event) & MP_EVENT_DIRECTION_UPSTREAM);
	bool is_downstream = (MP_EVENT_DIRECTION(event) & MP_EVENT_DIRECTION_DOWNSTREAM);

	/* Forward the event to the peer pad */
	if ((is_sink && is_upstream) || (is_src && is_downstream)) {
		return mp_pad_send_event(pad->peer, event);
	}

	/* Forward the event to other pads within the same element */
	struct mp_element *element = MP_ELEMENT(pad->object.container);
	struct mp_object *obj;
	sys_dlist_t *otherpad_list = NULL;

	if (is_sink && is_downstream) {
		otherpad_list = &element->srcpads;
	}

	if (is_src && is_upstream) {
		otherpad_list = &element->sinkpads;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(otherpad_list, obj, node) {
		ret |= mp_pad_send_event(MP_PAD(obj), event);
	}

	return ret;
}

bool mp_pad_send_event(struct mp_pad *pad, struct mp_event *event)
{
	if (pad == NULL || event == NULL || pad->eventfn == NULL) {
		return false;
	}

	return pad->eventfn(pad, event);
}
