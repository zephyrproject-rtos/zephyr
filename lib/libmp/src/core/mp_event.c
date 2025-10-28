/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#include "mp_event.h"

struct mp_event *mp_event_new_custom(enum mp_event_type type, struct mp_structure *structure)
{
	struct mp_event *event = (struct mp_event *)k_malloc(sizeof(struct mp_event));

	if (event == NULL) {
		return NULL;
	}

	event->type = type;
	event->structure = structure;
	event->timestamp = k_uptime_get_32();

	return event;
}

struct mp_event *mp_event_new_eos(void)
{
	return mp_event_new_custom(MP_EVENT_EOS, NULL);
}

struct mp_event *mp_event_new_caps(struct mp_caps *caps)
{

	return mp_event_new_custom(MP_EVENT_CAPS,
				   mp_structure_new(MP_MEDIA_UNKNOWN, MP_EVENT_CAPS, MP_TYPE_OBJECT,
						    caps, MP_EVENT_END));
}

void mp_event_destroy(struct mp_event *event)
{
	if (event) {
		mp_structure_destroy(event->structure);
		k_free(event);
	}
}

struct mp_caps *mp_event_get_caps(struct mp_event *event)
{
	if (event == NULL || event->type != MP_EVENT_CAPS) {
		return NULL;
	}

	return MP_CAPS(
		mp_value_get_object(mp_structure_get_value(event->structure, MP_EVENT_CAPS)));
}

bool mp_event_set_caps(struct mp_event *event, struct mp_caps *caps)
{
	if (event == NULL || event->type != MP_EVENT_CAPS || event->structure == NULL) {
		return false;
	}

	struct mp_value *value = mp_structure_get_value(event->structure, MP_EVENT_CAPS);

	if (value) {
		mp_value_set(value, MP_TYPE_OBJECT, caps);
	} else {
		mp_structure_append(event->structure, MP_EVENT_CAPS,
				    mp_value_new(MP_TYPE_OBJECT, caps));
	}

	return true;
}
