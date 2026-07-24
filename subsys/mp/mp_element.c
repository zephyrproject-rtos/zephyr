/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/mp_bin.h>
#include <zephyr/mp/mp_bus.h>
#include <zephyr/mp/mp_caps.h>
#include <zephyr/mp/mp_element.h>
#include <zephyr/mp/mp_dispatch.h>
#include <zephyr/mp/mp_message.h>
#include <zephyr/mp/mp_object.h>
#include <zephyr/mp/mp_pad.h>

LOG_MODULE_REGISTER(mp_element, CONFIG_MP_LOG_LEVEL);

/*
 * Optional, opt-in destructor for an element's base resources. It is NOT
 * called automatically during the play/pause/stop/replay lifecycle; it only
 * runs when the caller explicitly drops the element's last reference via
 * mp_object_unref(). It releases the caps stored on the element's pads (both
 * the template caps set at init time and any negotiated caps stored later via
 * mp_caps_replace). Derived element types that own extra caps chain to this
 * from their own release callback.
 */
void mp_element_release(struct mp_object *obj)
{
	struct mp_element *element = (struct mp_element *)obj;
	struct mp_object *pad_obj;
	struct mp_pad *pad;

	SYS_DLIST_FOR_EACH_CONTAINER(&element->srcpads, pad_obj, node) {
		pad = (struct mp_pad *)pad_obj;
		if (pad->caps != NULL) {
			mp_caps_unref(pad->caps);
			pad->caps = NULL;
		}
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&element->sinkpads, pad_obj, node) {
		pad = (struct mp_pad *)pad_obj;
		if (pad->caps != NULL) {
			mp_caps_unref(pad->caps);
			pad->caps = NULL;
		}
	}
}

void mp_element_add_pad(struct mp_element *element, struct mp_pad *pad)
{
	__ASSERT_NO_MSG(element != NULL);
	__ASSERT_NO_MSG(pad != NULL);

	/* Set element that contains this pad */
	pad->object.container = &element->object;

	if (pad->direction == MP_PAD_SRC) {
		sys_dlist_append(&element->srcpads, &pad->object.node);
	}

	if (pad->direction == MP_PAD_SINK) {
		sys_dlist_append(&element->sinkpads, &pad->object.node);
	}
}

static struct mp_pad *mp_element_get_unlinked_pad(struct mp_element *element, uint8_t pad_id,
						  enum mp_pad_direction direction)
{
	struct mp_object *obj;
	struct mp_pad *pad;

	sys_dlist_t *pads = direction == MP_PAD_SRC ? &element->srcpads : &element->sinkpads;

	SYS_DLIST_FOR_EACH_CONTAINER(pads, obj, node) {
		pad = (struct mp_pad *)obj;
		if (pad->peer == NULL && (pad_id == UINT8_MAX || pad_id == obj->id)) {
			return pad;
		}
	}

	return NULL;
}

static int mp_element_link_pads(struct mp_element *src, uint8_t src_pad_id, struct mp_element *sink,
				uint8_t sink_pad_id)
{
	struct mp_pad *srcpad = mp_element_get_unlinked_pad(src, src_pad_id, MP_PAD_SRC);
	struct mp_pad *sinkpad = mp_element_get_unlinked_pad(sink, sink_pad_id, MP_PAD_SINK);

	if (srcpad == NULL || sinkpad == NULL) {
		LOG_ERR("Link failed: no free %s pad on element %u",
			srcpad == NULL ? "src" : "sink",
			srcpad == NULL ? src->object.id : sink->object.id);
		return -EINVAL;
	}

	if (!mp_caps_can_intersect(srcpad->caps, sinkpad->caps)) {
		return -ENOTSUP;
	}

	return mp_pad_link(srcpad, sinkpad);
}

int mp_element_link(struct mp_element *element, struct mp_element *next_element, ...)
{
	int ret;
	va_list args;

	va_start(args, next_element);
	while (next_element != NULL) {
		/* Connect the 1st unlinked pad of each element together */
		ret = mp_element_link_pads(element, UINT8_MAX, next_element, UINT8_MAX);
		if (ret != 0) {
			va_end(args);
			return ret;
		}

		element = next_element;
		next_element = va_arg(args, struct mp_element *);
	}
	va_end(args);

	return 0;
}

enum mp_state_change_return mp_element_set_state(struct mp_element *element, enum mp_state state)
{
	if (element->set_state != NULL) {
		return element->set_state(element, state);
	}

	return MP_STATE_CHANGE_FAILURE;
}

static enum mp_state_change_return mp_element_set_state_func(struct mp_element *element,
							     enum mp_state state)
{
	enum mp_state_change_return ret = MP_STATE_CHANGE_SUCCESS;
	enum mp_state_change transition;
	enum mp_state next;
	enum mp_state *current = &element->current_state;

	while (*current != state) {
		next = MP_STATE_GET_NEXT(*current, state);
		transition = MP_STATE_TRANSITION(*current, next);
		ret = element->change_state(element, transition);
		/* Do not handle ASYNC yet */
		if (ret != MP_STATE_CHANGE_SUCCESS) {
			return ret;
		}

		*current = next;
	}

	return ret;
}

static enum mp_state_change_return mp_element_change_state_func(struct mp_element *element,
								enum mp_state_change transition)
{
	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		LOG_DBG("State changed READY -> PAUSED");
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		LOG_DBG("State changed PAUSED -> PLAYING");
		break;
	case MP_STATE_CHANGE_PLAYING_TO_PAUSED:
		LOG_DBG("State changed PLAYING -> PAUSED");
		break;
	case MP_STATE_CHANGE_PAUSED_TO_READY:
		LOG_DBG("State changed PAUSED -> READY");
		break;
	default:
		LOG_ERR("State changed UNKNOWN");
		break;
	}

	return MP_STATE_CHANGE_SUCCESS;
}

struct mp_bus *mp_element_get_bus(struct mp_element *element)
{
	if (element == NULL) {
		return NULL;
	}

	/* Get the top-level bin (i.e. pipeline) bus to send the message for now, but messages may
	 * be passed hierachically from the nearest bin to the pipeline if they need to be filtered
	 * or modified at each level.
	 */
	while (element->object.container != NULL) {
		element = (struct mp_element *)element->object.container;
	}

	return &((struct mp_bin *)element)->bus;
}

int mp_element_post_message(struct mp_element *element, uint32_t type)
{
	struct mp_bus *bus;
	struct mp_message msg;

	if (element == NULL) {
		return -EINVAL;
	}

	bus = mp_element_get_bus(element);
	if (bus == NULL) {
		return -ENODEV;
	}

	MP_MESSAGE_INIT(&msg, element, type);

	return mp_bus_post(bus, &msg);
}

void mp_element_init(struct mp_element *self, uint8_t id)
{
	self->object.id = id;

	/*
	 * The element is born with one reference held by its creator. Adding it
	 * to a bin transfers this reference to the bin, so a bin's teardown
	 * (mp_bin_release) drops it and triggers this optional release callback.
	 * The release callback is opt-in: nothing in the play/pause/stop/replay
	 * lifecycle calls mp_object_unref() on an element.
	 */
	self->object.ref = ATOMIC_INIT(1);
	self->object.release = mp_element_release;

	sys_dlist_init(&self->srcpads);
	sys_dlist_init(&self->sinkpads);

	self->current_state = MP_STATE_READY;
	self->set_state = mp_element_set_state_func;
	self->change_state = mp_element_change_state_func;
	self->eventfn = NULL;
}
