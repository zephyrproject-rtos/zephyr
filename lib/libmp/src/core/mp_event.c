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

MpEvent *mp_event_new_custom(MpEventType type, MpStructure *structure)
{
	MpEvent *event = (MpEvent *)k_malloc(sizeof(MpEvent));

	if (event == NULL) {
		return NULL;
	}

	event->type = type;
	event->structure = structure;
	event->timestamp = k_uptime_get_32();

	return event;
}

MpEvent *mp_event_new_eos(void)
{
	return mp_event_new_custom(MP_EVENT_EOS, NULL);
}

MpEvent *mp_event_new_caps(MpCaps *caps)
{

	return mp_event_new_custom(MP_EVENT_CAPS,
				   mp_structure_new(NULL, "caps", MP_TYPE_OBJECT, caps, NULL));
}

void mp_event_destroy(MpEvent *event)
{
	if (event) {
		mp_structure_destroy(event->structure);
		k_free(event);
	}
}

MpCaps *mp_event_get_caps(MpEvent *event)
{
	if (event == NULL || event->type != MP_EVENT_CAPS) {
		return NULL;
	}

	return MP_CAPS(mp_value_get_object(mp_structure_get_value(event->structure, "caps")));
}

bool mp_event_set_caps(MpEvent *event, MpCaps *caps)
{
	if (event == NULL || event->type != MP_EVENT_CAPS || event->structure == NULL) {
		return false;
	}

	MpValue *value = mp_structure_get_value(event->structure, "caps");

	if (value) {
		mp_value_set(value, MP_TYPE_OBJECT, caps);
	} else {
		mp_structure_append(event->structure, "caps", mp_value_new(MP_TYPE_OBJECT, caps));
	}

	return true;
}
