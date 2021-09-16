/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include "measurement_event.h"
#include "control_event.h"
#include "ack_event.h"

#define VALUE2_THRESH 15
#define MODULE controller

static bool ack_req;

void send_control_event(void)
{
	ack_req = true;
	struct control_event *event = new_control_event();

	EVENT_SUBMIT(event);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_measurement_event(eh)) {
		__ASSERT_NO_MSG(!ack_req);
		struct measurement_event *me = cast_measurement_event(eh);

		if ((me->value2 >= VALUE2_THRESH) ||
		    (me->value2 <= -VALUE2_THRESH)) {
			send_control_event();
		}

		return false;
	}

	if (is_ack_event(eh)) {
		__ASSERT_NO_MSG(ack_req);
		ack_req = false;
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, measurement_event);
EVENT_SUBSCRIBE(MODULE, ack_event);
