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

void mp_pad_init(MpPad *pad, const char *name, MpPadDirection direction, MpPadPresence presence,
		 MpCaps *caps)
{
	__ASSERT_NO_MSG(pad != NULL);

	pad->object.name = name;
	pad->direction = direction;
	pad->presence = presence;
	pad->caps = caps;
}

MpPad *mp_pad_new(const char *name, MpPadDirection direction, MpPadPresence presence, MpCaps *caps)
{
	MpPad *pad = k_malloc(sizeof(MpPad));

	mp_pad_init(pad, name, direction, presence, caps);

	return pad;
}

bool mp_pad_link(MpPad *srcpad, MpPad *sinkpad)
{
	int ret = true;

	if (srcpad == NULL || sinkpad == NULL) {
		return false;
	}

	/* Set peer pad */
	srcpad->peer = sinkpad;
	sinkpad->peer = srcpad;

	return ret;
}

bool mp_pad_start_task(MpPad *pad, MpTaskFunction func, int priority, void *user_data)
{
	k_tid_t thread = NULL;

	if (pad->task.running) {
		return false;
	}

	thread = mp_task_create(&pad->task, func, pad, NULL, NULL, priority);

	return thread != NULL;
}

bool mp_pad_chain(MpPad *pad, MpBuffer *buffer)
{
	return pad->chainfn(pad, buffer);
}

bool mp_pad_push(MpPad *pad, MpBuffer *buffer)
{
	return mp_pad_chain(pad->peer, buffer);
}

bool mp_pad_query(MpPad *pad, MpQuery *query)
{
	if (pad == NULL || query == NULL || pad->queryfn == NULL) {
		return false;
	}

	return pad->queryfn(pad, query);
}

bool mp_pad_send_event_default(MpPad *pad, MpEvent *event)
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
	MpElement *element = MP_ELEMENT_CAST(pad->object.container);
	MpObject *obj;
	MpPad *otherpad;

	if (is_sink && is_downstream) {
		SYS_DLIST_FOR_EACH_CONTAINER(&element->srcpads, obj, node) {
			otherpad = MP_PAD(obj);
			ret = mp_pad_send_event(otherpad, event);
		}
	} else if (is_src && is_upstream) {
		SYS_DLIST_FOR_EACH_CONTAINER(&element->sinkpads, obj, node) {
			otherpad = MP_PAD(obj);
			ret = mp_pad_send_event(otherpad, event);
		}
	}

	return ret;
}

bool mp_pad_send_event(MpPad *pad, MpEvent *event)
{
	if (pad == NULL || event == NULL || pad->eventfn == NULL) {
		return false;
	}

	return pad->eventfn(pad, event);
}
