/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "mp_element.h"
#include "mp_pad.h"

LOG_MODULE_REGISTER(mp_element, CONFIG_LIBMP_LOG_LEVEL);

void mp_element_add_pad(MpElement *element, MpPad *pad)
{
	__ASSERT_NO_MSG(element != NULL);
	__ASSERT_NO_MSG(pad != NULL);

	/* Set element that contains this pad */
	MP_OBJECT(pad)->container = MP_OBJECT(element);

	if (MP_PAD_IS_SRC(pad)) {
		sys_dlist_append(&element->srcpads, &pad->object.node);
	} else if (MP_PAD_IS_SINK(pad)) {
		sys_dlist_append(&element->sinkpads, &pad->object.node);
	}
}

static MpPad *mp_element_get_unlinked_pad(MpElement *element, const char *padname,
					  MpPadDirection direction)
{
	MpObject *obj;
	MpPad *pad;

	sys_dlist_t *pads = direction == MP_PAD_SRC ? &element->srcpads : &element->sinkpads;

	SYS_DLIST_FOR_EACH_CONTAINER(pads, obj, node) {
		pad = MP_PAD(obj);
		if (pad->peer == NULL &&
		    (padname == NULL || (padname == NULL && strcmp(obj->name, padname) == 0))) {
			return pad;
		}
	}

	return NULL;
}

bool mp_element_link_pads(MpElement *src, const char *srcpadname, MpElement *sink,
			  const char *sinkpadname)
{
	MpPad *srcpad = mp_element_get_unlinked_pad(src, srcpadname, MP_PAD_SRC);
	MpPad *sinkpad = mp_element_get_unlinked_pad(sink, sinkpadname, MP_PAD_SINK);

	if (mp_caps_can_intersect(srcpad->caps, sinkpad->caps)) {
		return mp_pad_link(srcpad, sinkpad);
	}

	return false;
}

bool mp_element_link(MpElement *element, MpElement *next_element, ...)
{
	bool ret = false;
	va_list args;

	va_start(args, next_element);
	while (next_element != NULL) {
		ret = mp_element_link_pads(element, NULL, next_element, NULL);
		if (!ret) {
			break;
		}

		element = next_element;
		next_element = va_arg(args, MpElement *);
	}
	va_end(args);

	return ret;
}

MpStateChangeReturn mp_element_set_state(MpElement *element, MpState state)
{
	if (element->set_state) {
		return element->set_state(element, state);
	}

	return MP_STATE_CHANGE_FAILURE;
}

static MpStateChangeReturn mp_element_set_state_func(MpElement *element, MpState state)
{
	MpStateChangeReturn ret = MP_STATE_CHANGE_SUCCESS;
	MpStateChange transition;
	MpState next;
	MpState *current = &MP_STATE_CURRENT(element);

	/* Currently, implement this in an iterative way, not recursive. */
	while (*current != state) {
		next = MP_STATE_GET_NEXT(*current, state);
		transition = MP_STATE_TRANSITION(*current, next);
		ret = element->change_state(element, transition);
		/* Do not handle ASYNC yet */
		if (ret != MP_STATE_CHANGE_SUCCESS) {
			return ret;
		}

		/* No need if change_state() already set it ? */
		*current = next;
	}

	return ret;
}

static MpStateChangeReturn mp_element_change_state_func(MpElement *element,
							MpStateChange transition)
{
	MpStateChangeReturn result = MP_STATE_CHANGE_SUCCESS;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		break;
	case MP_STATE_CHANGE_PLAYING_TO_PAUSED:
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		break;
	default:
		break;
	}

	return result;
}

bool mp_element_send_event_default(MpElement *element, MpEvent *event)
{
	MpObject *obj;
	bool ret = false;

	if (element == NULL || event == NULL) {
		return ret;
	}

	if (MP_EVENT_DIRECTION(event) & MP_EVENT_DIRECTION_UPSTREAM) {
		SYS_DLIST_FOR_EACH_CONTAINER(&element->sinkpads, obj, node) {
			MpPad *pad = MP_PAD(obj);
			ret = mp_pad_send_event(pad, event);
			if (!ret) {
				break;
			}
		}
	} else if (MP_EVENT_DIRECTION(event) & MP_EVENT_DIRECTION_DOWNSTREAM) {
		SYS_DLIST_FOR_EACH_CONTAINER(&element->srcpads, obj, node) {
			MpPad *pad = MP_PAD(obj);
			ret = mp_pad_send_event(pad, event);
			if (!ret) {
				break;
			}
		}
	}

	return ret;
}

void mp_element_init(MpElement *self)
{
	sys_dlist_init(&self->srcpads);
	sys_dlist_init(&self->sinkpads);

	self->current_state = MP_STATE_READY;
	self->set_state = mp_element_set_state_func;
	self->change_state = mp_element_change_state_func;
}

MpBus *mp_element_get_bus(MpElement *element)
{
	if (element == NULL) {
		return NULL;
	}

	return element->bus;
}
