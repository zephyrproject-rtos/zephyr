/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_sink.h"

LOG_MODULE_REGISTER(mp_sink, CONFIG_LIBMP_LOG_LEVEL);

int mp_sink_set_property(MpObject *obj, uint32_t key, const void *val)
{
	return 0;
}

int mp_sink_get_property(MpObject *obj, uint32_t key, void *val)
{
	return 0;
}

static bool mp_sink_set_caps(MpSink *sink, MpCaps *caps)
{
	if (sink == NULL || caps == NULL || sink->set_caps == NULL) {
		return false;
	}

	return sink->set_caps(sink, caps);
}

/* TODO */
static MpStateChangeReturn mp_sink_change_state(MpElement *self, MpStateChange transition)
{
	MpStateChangeReturn ret = MP_STATE_CHANGE_SUCCESS;

	switch (transition) {
	case MP_STATE_CHANGE_READY_TO_PAUSED:
		break;
	case MP_STATE_CHANGE_PAUSED_TO_PLAYING:
		break;
	default:
		break;
	}

	return ret;
}

bool mp_sink_propose_allocation_default(MpSink *self, MpQuery *query)
{
	return true;
}

static bool mp_sink_query(MpPad *pad, MpQuery *query)
{
	MpSink *self = MP_SINK(pad->object.container);
	MpCaps *caps_intersect, *query_caps;
	int ret;

	switch (query->type) {
	case MP_QUERY_CAPS:
		query_caps = mp_query_get_caps(query);
		if (query_caps) {
			caps_intersect = mp_caps_intersect(self->sinkpad.caps, query_caps);
			ret = mp_query_set_caps(query, caps_intersect);
			mp_caps_unref(caps_intersect);
			return ret;
		} else {
			return mp_query_set_caps(query, self->sinkpad.caps);
		}
	case MP_QUERY_ALLOCATION:
		return self->propose_allocation(self, query);
	default:
		return false;
	}
}

bool mp_sink_event(MpPad *pad, MpEvent *event)
{
	MpSink *sink = MP_SINK(pad->object.container);

	switch (MP_EVENT_TYPE(event)) {
	case MP_EVENT_EOS:
		LOG_DBG("MP_EVENT_EOS");
		return true;
	case MP_EVENT_CAPS:
		LOG_DBG("MP_EVENT_CAPS");
		return sink->set_caps(sink, mp_event_get_caps(event));
	default:
		return true;
	}
}

void mp_sink_init(MpElement *self)
{
	MpSink *sink = MP_SINK(self);

	/* Add pad */
	mp_pad_init(&sink->sinkpad, "sink", MP_PAD_SINK, MP_PAD_ALWAYS, NULL);
	mp_element_add_pad(self, &sink->sinkpad);

	sink->sinkpad.queryfn = mp_sink_query;
	sink->sinkpad.eventfn = mp_sink_event;

	self->object.set_property = mp_sink_set_property;
	self->object.get_property = mp_sink_get_property;
	self->change_state = mp_sink_change_state;

	sink->set_caps = mp_sink_set_caps;
	sink->propose_allocation = mp_sink_propose_allocation_default;
}
